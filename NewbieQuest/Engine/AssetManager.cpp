#include "AssetManager.h"
#include <filesystem>

namespace fs = std::filesystem;

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
    fs::path FullPath = MakeAssetPath(std::move(name));
    FullPath.replace_extension(".qasset_t");
    return SaveQAssetAsText(obj, FullPath.string());
}

std::unique_ptr<QObject> QAssetManager::LoadAssetFromText(std::string name)
{
    fs::path FullPath = MakeAssetPath(std::move(name));
    FullPath.replace_extension(".qasset_t");
    return LoadQAssetByText(FullPath.string());
}

bool QAssetManager::SaveAsset(const QObject& obj, const std::string name)
{
    auto FullPath = MakeAssetPath(std::move(name));
    return SaveQAsset(obj, FullPath.string());
}

std::unique_ptr<QObject> QAssetManager::LoadAssetBinary(const std::string name)
{
    auto full = MakeAssetPath(std::move(name));
    return LoadQAsset(full.string());
}

bool QAssetManager::SaveQAsset(const QObject& obj, const std::string& path)
{
    std::ofstream OutputStream(path, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!OutputStream) return false;

    constexpr char Magic[4] = {'Q','A','S','B'};
    OutputStream.write(Magic, 4);
    Write_Unsigned16(OutputStream, 2); // version 2: leaf-flat (structs flattened)
    Write_Unsigned16(OutputStream, 0);

    ClassInfo& ci = obj.GetClassInfo();
    WriteStream(OutputStream, ci.Name);
    WriteStream(OutputStream, obj.GetObjectName());

    // gather leaf 
    struct LeafRow { std::string Name; const PropertyBase* P; const void* Owner; };
    std::vector<LeafRow> rows;
    ForEachLeafConst(obj, [&](const std::string& full, const PropertyBase& leaf, const void* ownerPtr){
        rows.push_back({ full, &leaf, ownerPtr });
    });

    if (rows.size()>0xFFFF) throw std::runtime_error("too many leaf properties");
    Write_Unsigned16(OutputStream, (uint16_t)rows.size());

    for (auto& r : rows) {
        WriteStream(OutputStream, r.Name);
        uint8_t kind = (r.P->Kind == BasicKind::Bool) ? 0 : (r.P->Kind == BasicKind::Int) ? 1 : (r.P->Kind == BasicKind::Float) ? 2 : 0xFF;
        if (kind==0xFF) throw std::runtime_error("non-primitive leaf");
        Write_Unsigned8(OutputStream, kind);

        const void* vp = r.P->CPtr(r.Owner);
        switch (kind) {
        case 0: { uint8_t b = (*reinterpret_cast<const bool*>(vp)) ? 1u : 0u; Write_Unsigned8(OutputStream,b); } break;
        case 1: { int32_t v = (int32_t)(*reinterpret_cast<const int*>(vp)); Write_Int32(OutputStream,v); } break;
        case 2: { float v = *reinterpret_cast<const float*>(vp); Write_Float32(OutputStream,v); } break;
        default: { throw std::runtime_error("non-primitive leaf");}
        }
    }
    return bool(OutputStream);
}

std::unique_ptr<QObject> QAssetManager::LoadQAsset(const std::string& path)
{
    std::ifstream InputStream(path, std::ios::binary);
    if (!InputStream) return nullptr;

    char Magic[4]; InputStream.read(Magic,4);
    if (!InputStream || std::memcmp(Magic,"QASB",4)!=0) return nullptr;

    uint16_t Version=0, Reserved=0;
    if (!Read_Unsigned16(InputStream,Version) || !Read_Unsigned16(InputStream,Reserved)) return nullptr;
    if (Version != 2) return nullptr; // process only v2

    std::string className, objectName;
    if (!ReadStream(InputStream,className) || !ReadStream(InputStream,objectName)) return nullptr;

    ClassInfo* ci = Registry::Get().Find(className);
    if (!ci || !ci->Factory) return nullptr;

    std::unique_ptr<QObject> obj = ci->Factory();
    obj->SetObjectName(objectName);

    uint16_t count=0; if (!Read_Unsigned16(InputStream,count)) return nullptr;

    // leaf setter map
    std::unordered_map<std::string, const PropertyBase*> props;
    std::unordered_map<std::string, void*> owners;
    ForEachLeaf(*obj, [&](const std::string& full, const PropertyBase& leaf, void* ownerPtr){
        props[full]  = &leaf;
        owners[full] = ownerPtr;
    });

    for (uint16_t i=0; i<count; ++i) {
        std::string name; if (!ReadStream(InputStream,name)) return nullptr;
        uint8_t kind=0xFF; if (!Read_Unsigned8(InputStream,kind)) return nullptr;

        auto it = props.find(name);
        const PropertyBase* pb = (it==props.end()? nullptr : it->second);
        void* ownerPtr = (pb? owners[name] : nullptr);

        switch (kind) {
            case 0: { uint8_t b=0; if(!Read_Unsigned8(InputStream,b)) return nullptr;
                      if (pb && pb->Kind==BasicKind::Bool)  *reinterpret_cast<bool*>(pb->Ptr(ownerPtr))  = (b!=0); } break;
            case 1: { int32_t v=0; if(!Read_Int32(InputStream,v)) return nullptr;
                      if (pb && pb->Kind==BasicKind::Int)   *reinterpret_cast<int*>(pb->Ptr(ownerPtr))   = (int)v; } break;
            case 2: { float v=0;   if(!Read_Float32(InputStream,v)) return nullptr;
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
