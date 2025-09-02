#include "Binary.h"
#include <fstream>
#include <cstdint>
#include <bit>
#include <cstring>
#include <unordered_map>
#include "ClassInfo.h"
#include "Property.h"
#include "TypeTraits.h"
#include "Walk.h"
#include "Endian.h"

namespace qreflect {
namespace {

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
} // anon

bool SaveQAssetBinary(const QObject& obj, const std::string& path) {
    std::ofstream os(path, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!os) return false;

    const char magic[4] = {'Q','A','S','B'};
    os.write(magic, 4);
    write_u16(os, 2); // version 2: leaf-flat (structs flattened)
    write_u16(os, 0);

    ClassInfo& ci = obj.GetClassInfo();
    write_str(os, ci.Name);
    write_str(os, obj.GetObjectName());

    // leaf 수집
    struct LeafRow { std::string Name; const PropertyBase* P; const void* Owner; };
    std::vector<LeafRow> rows;
    ForEachLeafConst(obj, [&](const std::string& full, const PropertyBase& leaf, const void* ownerPtr){
        rows.push_back({ full, &leaf, ownerPtr });
    });

    if (rows.size()>0xFFFF) throw std::runtime_error("too many leaf properties");
    write_u16(os, (uint16_t)rows.size());

    for (auto& r : rows) {
        write_str(os, r.Name);
        uint8_t kind = (r.P->Kind == BasicKind::Bool) ? 0 : (r.P->Kind == BasicKind::Int) ? 1 : (r.P->Kind == BasicKind::Float) ? 2 : 0xFF;
        if (kind==0xFF) throw std::runtime_error("non-primitive leaf");
        write_u8(os, kind);

        const void* vp = r.P->CPtr(r.Owner);
        switch (kind) {
            case 0: { uint8_t b = (*reinterpret_cast<const bool*>(vp)) ? 1u : 0u; write_u8(os,b); } break;
            case 1: { int32_t v = (int32_t)(*reinterpret_cast<const int*>(vp)); write_i32(os,v); } break;
            case 2: { float v = *reinterpret_cast<const float*>(vp); write_f32(os,v); } break;
        }
    }
    return bool(os);
}

std::unique_ptr<QObject> LoadQAssetBinary(const std::string& path) {
    std::ifstream is(path, std::ios::binary);
    if (!is) return nullptr;

    char magic[4]; is.read(magic,4);
    if (!is || std::memcmp(magic,"QASB",4)!=0) return nullptr;

    uint16_t version=0,reserved=0;
    if (!read_u16(is,version) || !read_u16(is,reserved)) return nullptr;
    if (version != 2) return nullptr; // v2만 처리(원하면 v1 fallback 추가)

    std::string className, objectName;
    if (!read_str(is,className) || !read_str(is,objectName)) return nullptr;

    ClassInfo* ci = Registry::Get().Find(className);
    if (!ci || !ci->Factory) return nullptr;

    std::unique_ptr<QObject> obj = ci->Factory();
    obj->SetObjectName(objectName);

    uint16_t count=0; if (!read_u16(is,count)) return nullptr;

    // leaf setter 맵
    std::unordered_map<std::string, const PropertyBase*> props;
    std::unordered_map<std::string, void*> owners;
    ForEachLeaf(*obj, [&](const std::string& full, const PropertyBase& leaf, void* ownerPtr){
        props[full]  = &leaf;
        owners[full] = ownerPtr;
    });

    for (uint16_t i=0; i<count; ++i) {
        std::string name; if (!read_str(is,name)) return nullptr;
        uint8_t kind=0xFF; if (!read_u8(is,kind)) return nullptr;

        auto it = props.find(name);
        const PropertyBase* pb = (it==props.end()? nullptr : it->second);
        void* ownerPtr = (pb? owners[name] : nullptr);

        switch (kind) {
            case 0: { uint8_t b=0; if(!read_u8(is,b)) return nullptr;
                      if (pb && pb->Kind==BasicKind::Bool)  *reinterpret_cast<bool*>(pb->Ptr(ownerPtr))  = (b!=0); } break;
            case 1: { int32_t v=0; if(!read_i32(is,v)) return nullptr;
                      if (pb && pb->Kind==BasicKind::Int)   *reinterpret_cast<int*>(pb->Ptr(ownerPtr))   = (int)v; } break;
            case 2: { float v=0;   if(!read_f32(is,v)) return nullptr;
                      if (pb && pb->Kind==BasicKind::Float) *reinterpret_cast<float*>(pb->Ptr(ownerPtr)) = v; } break;
            default: return nullptr;
        }
    }
    return obj;
}

} // namespace qreflect
