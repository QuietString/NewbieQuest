#pragma once
#include <memory>
#include <fstream>
#include <string>
#include <functional>
#include "CoreMinimal.h"
#include "Reflection/Public/StructInfo.h"

#if __has_include(<bit>)
  #include <bit> // std::endian (C++20)
#endif

class QObject;

namespace fs = std::filesystem;

// Singleton class for managing assets.
class QAssetManager
{
public:

    static QAssetManager& Get() { static QAssetManager AssetManager; return AssetManager; }
    
    void SetAssetRoot(const fs::path& p);
    const fs::path& EnsureAssetRoot();
    fs::path MakeAssetPath(std::string name);  // append .qasset if missing

    bool SaveAssetByText(const QObject& obj, std::string name);
    std::unique_ptr<QObject> LoadAssetFromText(std::string name);

    bool SaveAsset(const QObject& obj, const std::string name);
    std::unique_ptr<QObject> LoadAssetBinary(const std::string name);

    template<typename T>
    requires std::is_base_of_v<QObject, T>
    inline bool SaveAsset(const std::unique_ptr<T>& p, const std::string& path)
    {
        return p ? SaveAsset(*p, path) : false;
    }
    // Little-endian binary .qasset
    // format v1:
    //  [4]  Magic "QASB"
    //  [2]  Version = 1
    //  [2]  Reserved = 0
    //  [2]  ClassNameLen
    //  [N]  ClassName (UTF-8)
    //  [2]  ObjectNameLen
    //  [N]  ObjectName (UTF-8)
    //  [2]  PropertyCount
    //  repeat PropertyCount count:
    //     [2] NameLen
    //     [N] Name (UTF-8)
    //     [1] TypeKind (0=Bool, 1=Int, 2=Float)
    //     [V] Value (Bool:1, Int:4, Float:4)
    bool SaveQAsset(const QObject& obj, const std::string& path);
    std::unique_ptr<QObject> LoadQAsset(const std::string& path);
    
    // text .qasset
    bool SaveQAssetAsText(const QObject& obj, const std::string& path);
    std::unique_ptr<QObject> LoadQAssetByText(const std::string& path);

    // debug dump
    void DumpObject(const QObject& obj, std::ostream& os);

    inline std::string trim(std::string s) {
        auto notspace = [](int ch){ return !std::isspace(ch); };
        s.erase(s.begin(), std::ranges::find_if(s, notspace));
        s.erase(std::find_if(s.rbegin(), s.rend(), notspace).base(), s.end());
        return s;
    }
#pragma region Endian

private:
    // byte swap(supporting 2/4/8)
    template <class T>
    inline T byteswap(T v) {
        static_assert(std::is_trivially_copyable_v<T>, "byteswap requires trivially copyable type");
        if constexpr (sizeof(T) == 1) {
            return v;
        } else if constexpr (sizeof(T) == 2) {
            uint16_t x; std::memcpy(&x, &v, 2);
            x = static_cast<uint16_t>((x << 8) | (x >> 8));
            std::memcpy(&v, &x, 2);
            return v;
        } else if constexpr (sizeof(T) == 4) {
            uint32_t x; std::memcpy(&x, &v, 4);
            x = ((x & 0x000000FFu) << 24) |
                ((x & 0x0000FF00u) << 8)  |
                ((x & 0x00FF0000u) >> 8)  |
                ((x & 0xFF000000u) >> 24);
            std::memcpy(&v, &x, 4);
            return v;
        } else if constexpr (sizeof(T) == 8) {
            uint64_t x; std::memcpy(&x, &v, 8);
            x =  (x >> 56)
               | ((x >> 40) & 0x000000000000FF00ull)
               | ((x >> 24) & 0x0000000000FF0000ull)
               | ((x >>  8) & 0x00000000FF000000ull)
               | ((x <<  8) & 0x000000FF00000000ull)
               | ((x << 24) & 0x0000FF0000000000ull)
               | ((x << 40) & 0x00FF000000000000ull)
               |  (x << 56);
            std::memcpy(&v, &x, 8);
            return v;
        } else {
            // extend it if needed
            return v;
        }
    }

    // little endian fixation save/restore
    template <class T>
    inline T to_le(T v) {
    #if defined(__cpp_lib_endian)
        if constexpr (std::endian::native == std::endian::little) {
            return v;
        } else {
            return byteswap(v);
        }
    #else
        #ifdef _WIN32
            return v; // MSVC/x86 uses little endian
        #else
            // conservative swap (adjustable as desired environment)
            return byteswap(v);
        #endif
    #endif
    }

    template <class T>
    inline T from_le(T v) {
        // same logic as to_le (symmetry)
    #if defined(__cpp_lib_endian)
        if constexpr (std::endian::native == std::endian::little) {
            return v;
        } else {
            return byteswap(v);
        }
    #else
        #ifdef _WIN32
            return v;
        #else
            return byteswap(v);
        #endif
    #endif
    }
#pragma endregion

#pragma region Binary I/O
    
private:
    inline void write_raw(std::ofstream& os, const void* p, std::size_t n) {
        os.write(reinterpret_cast<const char*>(p), static_cast<std::streamsize>(n));
    }
    inline void read_raw(std::ifstream& is, void* p, std::size_t n) {
        is.read(reinterpret_cast<char*>(p), static_cast<std::streamsize>(n));
    }

    inline void write_u8 (std::ofstream& os, uint8_t v){ write_raw(os, &v, 1); }
    inline bool  read_u8  (std::ifstream& is, uint8_t& v){ read_raw(is, &v, 1); return bool(is); }

    inline void write_u16(std::ofstream& os, uint16_t v){ v = to_le<uint16_t>(v); write_raw(os, &v, 2); }
    inline bool  read_u16 (std::ifstream& is, uint16_t& v){ read_raw(is, &v, 2); v = from_le<uint16_t>(v); return bool(is); }

    inline void write_i32(std::ofstream& os, int32_t v){ v = to_le<int32_t>(v); write_raw(os, &v, 4); }
    inline bool  read_i32 (std::ifstream& is, int32_t& v){ read_raw(is, &v, 4); v = from_le<int32_t>(v); return bool(is); }

    inline void write_f32(std::ofstream& os, float v){
        static_assert(sizeof(float)==4);
        uint32_t u; std::memcpy(&u, &v, 4);
        u = to_le<uint32_t>(u);
        write_raw(os, &u, 4);
    }
    inline bool  read_f32 (std::ifstream& is, float& v){
        static_assert(sizeof(float)==4);
        uint32_t u; read_raw(is, &u, 4);
        u = from_le<uint32_t>(u);
        std::memcpy(&v, &u, 4);
        return bool(is);
    }

    inline void write_str(std::ofstream& os, const std::string& s){
        if (s.size() > 0xFFFF) throw std::runtime_error("string too long");
        write_u16(os, static_cast<uint16_t>(s.size()));
        if (!s.empty()) write_raw(os, s.data(), s.size());
    }
    inline bool read_str(std::ifstream& is, std::string& s){
        uint16_t n=0; if (!read_u16(is, n)) return false;
        s.resize(n);
        if (n) read_raw(is, s.data(), n);
        return bool(is);
    }
#pragma endregion

#pragma region Walk down to leaf

private:
// recursively walk down to leaf(primitive type) and call back
// emit(name, leafProperty, ownerPtrOfLeaf)
    
template <typename Emit>
inline void ForEachLeafConst(const QObject& obj, const Emit& emit) {
    std::function<void(const void*, const qreflect::StructInfo&, const std::string&)> walkStruct;
    walkStruct = [&](const void* structPtr, const qreflect::StructInfo& si, const std::string& prefix){
        si.ForEachProperty([&](const qreflect::PropertyBase& sp){
            if (sp.Kind == qreflect::BasicKind::Struct && sp.GetStructInfo()) {
                const void* nested = sp.CPtr(structPtr);
                walkStruct(nested, *sp.GetStructInfo(), prefix + sp.Name + ".");
            } else {
                emit(prefix + sp.Name, sp, structPtr);
            }
        });
    };

    obj.GetClassInfo().ForEachProperty([&](const qreflect::PropertyBase& p){
        if (p.Kind == qreflect::BasicKind::Struct && p.GetStructInfo()) {
            const void* s = p.CPtr(&obj);
            walkStruct(s, *p.GetStructInfo(), p.Name + ".");
        } else {
            emit(p.Name, p, &obj);
        }
    });
}
    
// non-const version: used for creating setter map
// emit(name, leafProperty, ownerPtrOfLeaf)
template <typename Emit>
inline void ForEachLeaf(QObject& obj, const Emit& emit) {
    std::function<void(void*, const qreflect::StructInfo&, const std::string&)> walkStruct;
    walkStruct = [&](void* structPtr, const qreflect::StructInfo& si, const std::string& prefix){
        si.ForEachProperty([&](const qreflect::PropertyBase& sp){
            if (sp.Kind == qreflect::BasicKind::Struct && sp.GetStructInfo()) {
                void* nested = sp.Ptr(structPtr);
                walkStruct(nested, *sp.GetStructInfo(), prefix + sp.Name + ".");
            } else {
                emit(prefix + sp.Name, sp, structPtr);
            }
        });
    };

    obj.GetClassInfo().ForEachProperty([&](const qreflect::PropertyBase& p){
        if (p.Kind == qreflect::BasicKind::Struct && p.GetStructInfo()) {
            void* s = p.Ptr(&obj);
            walkStruct(s, *p.GetStructInfo(), p.Name + ".");
        } else {
            emit(p.Name, p, &obj);
        }
    });
}
#pragma endregion
    
};
