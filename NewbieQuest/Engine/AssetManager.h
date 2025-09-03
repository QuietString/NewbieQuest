#pragma once
#include <memory>
#include <fstream>
#include <string>
#include <functional>
#include "CoreMinimal.h"

#if __has_include(<bit>)
  #include <bit> // std::endian (C++20)
#endif

class QObject;

namespace FileSystem = std::filesystem;

// Singleton class for managing assets.
class QAssetManager
{
public:

    static QAssetManager& Get() { static QAssetManager AssetManager; return AssetManager; }
    
    void SetAssetRoot(const FileSystem::path& Path);
    const FileSystem::path& EnsureAssetRoot();
    FileSystem::path MakeAssetPath(std::string Name);  // append .qasset if missing

    bool SaveAssetByText(const QObject& Obj, std::string Name);
    std::unique_ptr<QObject> LoadAssetFromText(std::string Name);

    bool SaveAsset(const QObject& obj, const std::string Name);
    std::unique_ptr<QObject> LoadAssetBinary(const std::string Name);

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
    bool SaveQAsset(const QObject& Obj, const std::string& Path);
    std::unique_ptr<QObject> LoadQAsset(const std::string& Path);
    
    // text .qasset
    bool SaveQAssetAsText(const QObject& Obj, const std::string& Path);
    std::unique_ptr<QObject> LoadQAssetByText(const std::string& Path);

    // debug dump
    void DumpObject(const QObject& Obj, std::ostream& OutputStream);

    inline std::string Trim(std::string s) {
        auto notspace = [](int ch){ return !std::isspace(ch); };
        s.erase(s.begin(), std::ranges::find_if(s, notspace));
        s.erase(std::find_if(s.rbegin(), s.rend(), notspace).base(), s.end());
        return s;
    }
#pragma region Endian

private:
    // byte swap(supporting 2/4/8)
    template <class T>
    inline T ByteSwap(T v) {
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
    inline T ToLittleEndian(T v) {
    #if defined(__cpp_lib_endian)
        if constexpr (std::endian::native == std::endian::little) {
            return v;
        } else {
            return ByteSwap(v);
        }
    #else
        #ifdef _WIN32
            return v; // MSVC/x86 uses little endian
        #else
            // conservative swap (adjustable as desired environment)
            return ByteSwap(v);
        #endif
    #endif
    }

    template <class T>
    inline T FromLittleEndian(T v) {
        // same logic as to_le (symmetry)
    #if defined(__cpp_lib_endian)
        if constexpr (std::endian::native == std::endian::little) {
            return v;
        } else {
            return ByteSwap(v);
        }
    #else
        #ifdef _WIN32
            return v;
        #else
            return ByteSwap(v);
        #endif
    #endif
    }
#pragma endregion

#pragma region Binary I/O
    
private:
    inline void WriteRaw(std::ofstream& OutputStream, const void* Ptr, std::size_t n) {
        OutputStream.write(reinterpret_cast<const char*>(Ptr), static_cast<std::streamsize>(n));
    }
    inline void ReadRaw(std::ifstream& InputStream, void* Ptr, std::size_t n) {
        InputStream.read(reinterpret_cast<char*>(Ptr), static_cast<std::streamsize>(n));
    }

    inline void Write_Unsigned8 (std::ofstream& OutputStream, uint8_t v){ WriteRaw(OutputStream, &v, 1); }
    inline bool Read_Unsigned8 (std::ifstream& InputStream, uint8_t& v){ ReadRaw(InputStream, &v, 1); return bool(InputStream); }

    inline void Write_Unsigned16 (std::ofstream& OutputStream, uint16_t v){ v = ToLittleEndian<uint16_t>(v); WriteRaw(OutputStream, &v, 2); }
    inline bool Read_Unsigned16 (std::ifstream& InputStream, uint16_t& v){ ReadRaw(InputStream, &v, 2); v = FromLittleEndian<uint16_t>(v); return bool(InputStream); }

    inline void Write_Int32 (std::ofstream& OutputStream, int32_t v){ v = ToLittleEndian<int32_t>(v); WriteRaw(OutputStream, &v, 4); }
    inline bool Read_Int32 (std::ifstream& InputStream, int32_t& v){ ReadRaw(InputStream, &v, 4); v = FromLittleEndian<int32_t>(v); return bool(InputStream); }

    inline void Write_Float32 (std::ofstream& OutputStream, float v){
        static_assert(sizeof(float)==4);
        uint32_t u; std::memcpy(&u, &v, 4);
        u = ToLittleEndian<uint32_t>(u);
        WriteRaw(OutputStream, &u, 4);
    }
    inline bool  Read_Float32 (std::ifstream& InputStream, float& v){
        static_assert(sizeof(float)==4);
        uint32_t u; ReadRaw(InputStream, &u, 4);
        u = FromLittleEndian<uint32_t>(u);
        std::memcpy(&v, &u, 4);
        return bool(InputStream);
    }

    inline void WriteStream(std::ofstream& os, const std::string& s){
        if (s.size() > 0xFFFF) throw std::runtime_error("string too long");
        Write_Unsigned16(os, static_cast<uint16_t>(s.size()));
        if (!s.empty()) WriteRaw(os, s.data(), s.size());
    }
    inline bool ReadStream(std::ifstream& is, std::string& s){
        uint16_t n=0; if (!Read_Unsigned16(is, n)) return false;
        s.resize(n);
        if (n) ReadRaw(is, s.data(), n);
        return bool(is);
    }
#pragma endregion

#pragma region Walk down to leaf

private:
// recursively walk down to leaf(primitive type) and call back
// emit(Name, LeafProperty, OwnerPtrOfLeaf)
    
template <typename Emit>
inline void ForEachLeafConst(const QObject& Obj, const Emit& emit) {
    std::function<void(const void*, const StructInfo&, const std::string&)> WalkStruct;
    WalkStruct = [&](const void* StructPtr, const StructInfo& Si, const std::string& Prefix){
        Si.ForEachProperty([&](const PropertyBase& Sp){
            if (Sp.Kind == BasicKind::Struct && Sp.GetStructInfo()) {
                const void* nested = Sp.CPtr(StructPtr);
                WalkStruct(nested, *Sp.GetStructInfo(), Prefix + Sp.Name + ".");
            } else {
                emit(Prefix + Sp.Name, Sp, StructPtr);
            }
        });
    };

    Obj.GetClassInfo().ForEachProperty([&](const PropertyBase& p){
        if (p.Kind == BasicKind::Struct && p.GetStructInfo()) {
            const void* s = p.CPtr(&Obj);
            WalkStruct(s, *p.GetStructInfo(), p.Name + ".");
        } else {
            emit(p.Name, p, &Obj);
        }
    });
}
    
// non-const version: used for creating setter map
// emit(name, leafProperty, ownerPtrOfLeaf)
template <typename Emit>
inline void ForEachLeaf(QObject& Obj, const Emit& emit) {
    std::function<void(void*, const StructInfo&, const std::string&)> WalkStruct;
    WalkStruct = [&](void* StructPtr, const StructInfo& Si, const std::string& Prefix){
        Si.ForEachProperty([&](const PropertyBase& Sp){
            if (Sp.Kind == BasicKind::Struct && Sp.GetStructInfo()) {
                void* nested = Sp.Ptr(StructPtr);
                WalkStruct(nested, *Sp.GetStructInfo(), Prefix + Sp.Name + ".");
            } else {
                emit(Prefix + Sp.Name, Sp, StructPtr);
            }
        });
    };

    Obj.GetClassInfo().ForEachProperty([&](const PropertyBase& p){
        if (p.Kind == BasicKind::Struct && p.GetStructInfo()) {
            void* s = p.Ptr(&Obj);
            WalkStruct(s, *p.GetStructInfo(), p.Name + ".");
        } else {
            emit(p.Name, p, &Obj);
        }
    });
}
#pragma endregion
    
};
