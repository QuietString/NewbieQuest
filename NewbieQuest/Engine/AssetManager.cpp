#include "AssetManager.h"

#include <filesystem>
#include <fstream>
#include <unordered_map>
#include "Reflection/Public/TypeTraits.h"
#include "Reflection/Public/ClassInfo.h"
#include "Reflection/Public/Property.h"

namespace fs = std::filesystem;

using namespace qreflect;

static fs::path& RootStorage() {
    static fs::path root;
    return root;
}

void QAssetManager::SetAssetRoot(const fs::path& p)
{
    RootStorage() = p; 
}

const fs::path& QAssetManager::EnsureAssetRoot()
{
    auto& r = RootStorage();
    if (r.empty()) r = fs::current_path() / "Contents";
    fs::create_directories(r);
    return r;
}

fs::path QAssetManager::MakeAssetPath(std::string name)
{
    if (name.size() < 7 || name.substr(name.size() - 7) != ".qasset")
        name += ".qasset";
    fs::path full = EnsureAssetRoot() / name;
    fs::create_directories(full.parent_path());
    return full;
}

bool QAssetManager::SaveAssetByText(const QObject& obj, std::string name)
{
    fs::path full = MakeAssetPath(std::move(name));
    full.replace_extension(".qasset_t");
    return SaveQAssetAsText(obj, full.string());
}

std::unique_ptr<QObject> QAssetManager::LoadAssetFromText(std::string name)
{
    fs::path full = MakeAssetPath(std::move(name));
    full.replace_extension(".qasset_t");
    return LoadQAssetByText(full.string());
}

bool QAssetManager::SaveAsset(const QObject& obj, const std::string name)
{
    auto full = MakeAssetPath(std::move(name));
    return SaveQAsset(obj, full.string());
}

std::unique_ptr<QObject> QAssetManager::LoadAssetBinary(const std::string name)
{
    auto full = MakeAssetPath(std::move(name));
    return LoadQAsset(full.string());
}

bool QAssetManager::SaveQAsset(const QObject& obj, const std::string& path)
{
    std::ofstream os(path, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!os) return false;

    const char magic[4] = {'Q','A','S','B'};
    os.write(magic, 4);
    write_u16(os, 2); // version 2: leaf-flat (structs flattened)
    write_u16(os, 0);

    ClassInfo& ci = obj.GetClassInfo();
    write_str(os, ci.Name);
    write_str(os, obj.GetObjectName());

    // gather leaf 
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

std::unique_ptr<QObject> QAssetManager::LoadQAsset(const std::string& path)
{
    std::ifstream is(path, std::ios::binary);
    if (!is) return nullptr;

    char magic[4]; is.read(magic,4);
    if (!is || std::memcmp(magic,"QASB",4)!=0) return nullptr;

    uint16_t version=0,reserved=0;
    if (!read_u16(is,version) || !read_u16(is,reserved)) return nullptr;
    if (version != 2) return nullptr; // process only v2

    std::string className, objectName;
    if (!read_str(is,className) || !read_str(is,objectName)) return nullptr;

    ClassInfo* ci = Registry::Get().Find(className);
    if (!ci || !ci->Factory) return nullptr;

    std::unique_ptr<QObject> obj = ci->Factory();
    obj->SetObjectName(objectName);

    uint16_t count=0; if (!read_u16(is,count)) return nullptr;

    // leaf setter map
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

bool QAssetManager::SaveQAssetAsText(const QObject& obj, const std::string& path) {
    std::ofstream ofs(path, std::ios::out | std::ios::trunc);
    if (!ofs) return false;

    ClassInfo& ci = obj.GetClassInfo();
    ofs << "Class=" << ci.Name << "\n";
    ofs << "ObjectName=" << obj.GetObjectName() << "\n";

    // output leaf only: Foo.X:float=1.0
    ForEachLeafConst(obj, [&](const std::string& full, const PropertyBase& leaf, const void* ownerPtr){
        ofs << full << ":" << leaf.TypeName << "=" << leaf.GetAsString(ownerPtr) << "\n";
    });
    return true;
}

std::unique_ptr<QObject> QAssetManager::LoadQAssetByText(const std::string& path)
{
    std::ifstream ifs(path);
    if (!ifs) return nullptr;

    std::string line, className, objectName;

    if (!std::getline(ifs, line)) return nullptr;
    {
        auto pos = line.find('='); if (pos==std::string::npos || line.substr(0,pos)!="Class") return nullptr;
        className = trim(line.substr(pos+1));
    }
    if (!std::getline(ifs, line)) return nullptr;
    {
        auto pos = line.find('='); if (pos==std::string::npos || line.substr(0,pos)!="ObjectName") return nullptr;
        objectName = trim(line.substr(pos+1));
    }

    ClassInfo* ci = Registry::Get().Find(className);
    if (!ci || !ci->Factory) return nullptr;

    std::unique_ptr<QObject> obj = ci->Factory();
    obj->SetObjectName(objectName);

    // leaf setter map: "Foo.X" -> (leaf property, ownerPtr(=Foo struct address))
    std::unordered_map<std::string, const PropertyBase*> props;
    std::unordered_map<std::string, void*> owners;

    ForEachLeaf(*obj, [&](const std::string& full, const PropertyBase& leaf, void* ownerPtr){
        props[full]  = &leaf;
        owners[full] = ownerPtr;
    });

    // name:type=value
    while (std::getline(ifs, line)) {
        line = trim(line); if (line.empty()) continue;
        auto pos1 = line.find(':'), pos2 = line.find('=');
        if (pos1==std::string::npos || pos2==std::string::npos || pos1>pos2) continue;

        std::string pname = trim(line.substr(0, pos1));
        std::string tname = trim(line.substr(pos1+1, pos2-(pos1+1)));
        std::string value = trim(line.substr(pos2+1));

        auto itp = props.find(pname);
        if (itp == props.end()) continue;

        const PropertyBase* pb = itp->second;
        if (pb->TypeName != tname) continue;

        void* ownerPtr = owners[pname];
        pb->SetFromString(ownerPtr, value);
    }
    
    return obj;
}

void QAssetManager::DumpObject(const QObject& obj, std::ostream& os)
{
    ClassInfo& ci = obj.GetClassInfo();
    os << "[Class] " << ci.Name << "\n";
    os << "[ObjectName] " << obj.GetObjectName() << "\n";
    os << "[Properties]\n";

    ForEachLeafConst(obj, [&](const std::string& full, const PropertyBase& leaf, const void* ownerPtr){
        os << "  - " << leaf.TypeName << " " << full << " = " << leaf.GetAsString(ownerPtr) << "\n";
    });
}
