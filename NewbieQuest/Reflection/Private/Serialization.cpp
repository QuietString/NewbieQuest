#include "../Public/Serialization.h"
#include <fstream>
#include <unordered_map>
#include "../Public/Util.h"
#include "../Public/TypeTraits.h"
#include "../Public/ClassInfo.h"
#include "../Public/Property.h"
#include "../Public/Walk.h"

namespace qreflect {

bool SaveQAsset(const QObject& obj, const std::string& path) {
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

std::unique_ptr<QObject> LoadQAsset(const std::string& path) {
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

void DumpObject(const QObject& obj, std::ostream& os) {
    ClassInfo& ci = obj.GetClassInfo();
    os << "[Class] " << ci.Name << "\n";
    os << "[ObjectName] " << obj.GetObjectName() << "\n";
    os << "[Properties]\n";

    ForEachLeafConst(obj, [&](const std::string& full, const PropertyBase& leaf, const void* ownerPtr){
        os << "  - " << leaf.TypeName << " " << full << " = " << leaf.GetAsString(ownerPtr) << "\n";
    });
}

}
