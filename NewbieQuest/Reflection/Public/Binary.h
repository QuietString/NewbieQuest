#pragma once
#include <memory>
#include <string>
#include "QObject.h"

namespace qreflect {

    // Little-endian 바이너리 .qassetb
    // 포맷 v1:
    //  [4]  Magic "QASB"
    //  [2]  Version = 1
    //  [2]  Reserved = 0
    //  [2]  ClassNameLen
    //  [N]  ClassName (UTF-8)
    //  [2]  ObjectNameLen
    //  [N]  ObjectName (UTF-8)
    //  [2]  PropertyCount
    //  반복 PropertyCount 회:
    //     [2] NameLen
    //     [N] Name (UTF-8)
    //     [1] TypeKind (0=Bool, 1=Int, 2=Float)
    //     [V] Value (Bool:1, Int:4, Float:4)
    bool SaveQAssetBinary(const QObject& obj, const std::string& path);
    std::unique_ptr<QObject> LoadQAssetBinary(const std::string& path);

}
