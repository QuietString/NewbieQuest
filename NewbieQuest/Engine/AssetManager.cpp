#include "AssetManager.h"
#include <filesystem>

namespace FileSystem = std::filesystem;

static FileSystem::path& RootStorage() {
    static FileSystem::path Root;
    return Root;
}

void QAssetManager::SetAssetRoot(const FileSystem::path& Path)
{
    RootStorage() = Path; 
}

const FileSystem::path& QAssetManager::EnsureAssetRoot()
{
    auto& Root = RootStorage();
    if (Root.empty()) Root = FileSystem::current_path() / "Contents";
    FileSystem::create_directories(Root);
    return Root;
}

FileSystem::path QAssetManager::MakeAssetPath(std::string Name)
{
    if (Name.size() < 7 || Name.substr(Name.size() - 7) != ".qasset")
        Name += ".qasset";
    FileSystem::path FullPath = EnsureAssetRoot() / Name;
    FileSystem::create_directories(FullPath.parent_path());
    return FullPath;
}

bool QAssetManager::SaveAssetByText(const QObject& Obj, std::string Name)
{
    FileSystem::path FullPath = MakeAssetPath(std::move(Name));
    FullPath.replace_extension(".qasset_t");
    return SaveQAssetAsText(Obj, FullPath.string());
}

std::unique_ptr<QObject> QAssetManager::LoadAssetFromText(const std::string Name)
{
    FileSystem::path FullPath = MakeAssetPath(Name);
    FullPath.replace_extension(".qasset_t");
    return LoadQAssetByText(FullPath.string());
}

bool QAssetManager::SaveAsset(const QObject& Obj, const std::string Name)
{
    auto FullPath = MakeAssetPath(Name);
    return SaveQAsset(Obj, FullPath.string());
}

std::unique_ptr<QObject> QAssetManager::LoadAssetBinary(const std::string Name)
{
    auto FullPath = MakeAssetPath(Name);
    return LoadQAsset(FullPath.string());
}

bool QAssetManager::SaveQAsset(const QObject& Obj, const std::string& Path)
{
    std::ofstream OutputStream(Path, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!OutputStream) return false;

    constexpr char Magic[4] = {'Q','A','S','B'};
    OutputStream.write(Magic, 4);
    Write_Unsigned16(OutputStream, 2); // version 2: leaf-flat (structs flattened)
    Write_Unsigned16(OutputStream, 0);

    ClassInfo& Info = Obj.GetClassInfo();
    WriteStream(OutputStream, Info.Name);
    WriteStream(OutputStream, Obj.GetObjectName());

    // gather leaf 
    struct LeafRow { std::string Name; const PropertyBase* P; const void* Owner; };
    std::vector<LeafRow> Rows;
    ForEachLeafConst(Obj, [&](const std::string& full, const PropertyBase& Leaf, const void* OwnerPtr){
        Rows.push_back({ full, &Leaf, OwnerPtr });
    });

    if (Rows.size()>0xFFFF) throw std::runtime_error("too many leaf properties");
    Write_Unsigned16(OutputStream, (uint16_t)Rows.size());

    for (auto& Row : Rows) {
        WriteStream(OutputStream, Row.Name);
        uint8_t Kind = (Row.P->Kind == BasicKind::Bool) ? 0 : (Row.P->Kind == BasicKind::Int) ? 1 : (Row.P->Kind == BasicKind::Float) ? 2 : 0xFF;
        if (Kind==0xFF) throw std::runtime_error("non-primitive leaf");
        Write_Unsigned8(OutputStream, Kind);

        const void* VoidPtr = Row.P->ConstPtr(Row.Owner);
        switch (Kind) {
        case 0: {uint8_t b = (*reinterpret_cast<const bool*>(VoidPtr)) ? 1u : 0u; Write_Unsigned8(OutputStream,b); } break;
        case 1: { int32_t v = (int32_t)(*reinterpret_cast<const int*>(VoidPtr)); Write_Int32(OutputStream,v); } break;
        case 2: { float v = *reinterpret_cast<const float*>(VoidPtr); Write_Float32(OutputStream,v); } break;
        default: { throw std::runtime_error("non-primitive leaf");}
        }
    }
    return bool(OutputStream);
}

std::unique_ptr<QObject> QAssetManager::LoadQAsset(const std::string& Path)
{
    std::ifstream InputStream(Path, std::ios::binary);
    if (!InputStream) return nullptr;

    char Magic[4]; InputStream.read(Magic,4);
    if (!InputStream || std::memcmp(Magic,"QASB",4)!=0) return nullptr;

    uint16_t Version=0, Reserved=0;
    if (!Read_Unsigned16(InputStream,Version) || !Read_Unsigned16(InputStream,Reserved)) return nullptr;
    if (Version != 2) return nullptr; // process only v2

    std::string ClassName, ObjectName;
    if (!ReadStream(InputStream,ClassName) || !ReadStream(InputStream,ObjectName)) return nullptr;

    ClassInfo* Info = Registry::Get().Find(ClassName);
    if (!Info || !Info->Factory) return nullptr;

    std::unique_ptr<QObject> Obj = Info->Factory();
    Obj->SetObjectName(ObjectName);

    uint16_t count=0; if (!Read_Unsigned16(InputStream,count)) return nullptr;

    // leaf setter map
    std::unordered_map<std::string, const PropertyBase*> Properties;
    std::unordered_map<std::string, void*> Owners;
    ForEachLeaf(*Obj, [&](const std::string& full, const PropertyBase& Leaf, void* OwnerPtr){
        Properties[full]  = &Leaf;
        Owners[full] = OwnerPtr;
    });

    for (uint16_t i=0; i<count; ++i) {
        std::string Name; if (!ReadStream(InputStream,Name)) return nullptr;
        uint8_t Kind=0xFF; if (!Read_Unsigned8(InputStream,Kind)) return nullptr;

        auto It = Properties.find(Name);
        const PropertyBase* Pb = (It==Properties.end()? nullptr : It->second);
        void* OwnerPtr = (Pb? Owners[Name] : nullptr);

        switch (Kind) {
            case 0: { uint8_t b=0; if(!Read_Unsigned8(InputStream,b)) return nullptr;
                      if (Pb && Pb->Kind==BasicKind::Bool)  *reinterpret_cast<bool*>(Pb->Ptr(OwnerPtr))  = (b!=0); } break;
            case 1: { int32_t v=0; if(!Read_Int32(InputStream,v)) return nullptr;
                      if (Pb && Pb->Kind==BasicKind::Int)   *reinterpret_cast<int*>(Pb->Ptr(OwnerPtr))   = (int)v; } break;
            case 2: { float v=0;   if(!Read_Float32(InputStream,v)) return nullptr;
                      if (Pb && Pb->Kind==BasicKind::Float) *reinterpret_cast<float*>(Pb->Ptr(OwnerPtr)) = v; } break;
            default: return nullptr;
        }
    }
    return Obj;
}

bool QAssetManager::SaveQAssetAsText(const QObject& Obj, const std::string& Path) {
    std::ofstream OutputStream(Path, std::ios::out | std::ios::trunc);
    if (!OutputStream) return false;

    ClassInfo& Info = Obj.GetClassInfo();
    OutputStream << "Class=" << Info.Name << "\n";
    OutputStream << "ObjectName=" << Obj.GetObjectName() << "\n";

    // output leaf only: Foo.X:float=1.0
    ForEachLeafConst(Obj, [&](const std::string& full, const PropertyBase& Leaf, const void* OwnerPtr){
        OutputStream << full << ":" << Leaf.TypeName << "=" << Leaf.GetAsString(OwnerPtr) << "\n";
    });
    return true;
}

std::unique_ptr<QObject> QAssetManager::LoadQAssetByText(const std::string& Path)
{
    std::ifstream InputStream(Path);
    if (!InputStream) return nullptr;

    std::string Line, ClassName, ObjectName;

    if (!std::getline(InputStream, Line)) return nullptr;
    {
        auto Pos = Line.find('='); if (Pos==std::string::npos || Line.substr(0,Pos)!="Class") return nullptr;
        ClassName = Trim(Line.substr(Pos+1));
    }
    if (!std::getline(InputStream, Line)) return nullptr;
    {
        auto Pos = Line.find('='); if (Pos==std::string::npos || Line.substr(0,Pos)!="ObjectName") return nullptr;
        ObjectName = Trim(Line.substr(Pos+1));
    }

    ClassInfo* Info = Registry::Get().Find(ClassName);
    if (!Info || !Info->Factory) return nullptr;

    std::unique_ptr<QObject> Obj = Info->Factory();
    Obj->SetObjectName(ObjectName);

    // leaf setter map: "Foo.X" -> (leaf property, ownerPtr(=Foo struct address))
    std::unordered_map<std::string, const PropertyBase*> Props;
    std::unordered_map<std::string, void*> Owners;

    ForEachLeaf(*Obj, [&](const std::string& full, const PropertyBase& Leaf, void* OwnerPtr){
        Props[full]  = &Leaf;
        Owners[full] = OwnerPtr;
    });

    // name:type=value
    while (std::getline(InputStream, Line)) {
        Line = Trim(Line); if (Line.empty()) continue;
        auto Pos1 = Line.find(':'), Pos2 = Line.find('=');
        if (Pos1==std::string::npos || Pos2==std::string::npos || Pos1>Pos2) continue;

        std::string Pname = Trim(Line.substr(0, Pos1));
        std::string Tname = Trim(Line.substr(Pos1+1, Pos2-(Pos1+1)));
        std::string Value = Trim(Line.substr(Pos2+1));

        auto Itp = Props.find(Pname);
        if (Itp == Props.end()) continue;

        const PropertyBase* Pb = Itp->second;
        if (Pb->TypeName != Tname) continue;

        void* OwnerPtr = Owners[Pname];
        Pb->SetFromString(OwnerPtr, Value);
    }
    
    return Obj;
}

void QAssetManager::DumpObject(const QObject& Obj, std::ostream& OutputStream)
{
    ClassInfo& Info = Obj.GetClassInfo();
    OutputStream << "[Class] " << Info.Name << "\n";
    OutputStream << "[ObjectName] " << Obj.GetObjectName() << "\n";
    OutputStream << "[Properties]\n";

    ForEachLeafConst(Obj, [&](const std::string& FullPath, const PropertyBase& Leaf, const void* OwnerPtr){
        OutputStream << "  - " << Leaf.TypeName << " " << FullPath << " = " << Leaf.GetAsString(OwnerPtr) << "\n";
    });
}
