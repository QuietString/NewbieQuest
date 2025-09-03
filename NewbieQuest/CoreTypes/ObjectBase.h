#pragma once
#include <string>

#include "Reflection/Public/TypeInfos.h"

class QObjectBase
{
public:
    virtual ~QObjectBase() = default;

    const std::string& GetObjectName() const { return ObjectName; }
    void SetObjectName(std::string InName) { ObjectName = std::move(InName); }

    virtual ClassInfo& GetClassInfo() const = 0;
    static ClassInfo& StaticClass(); // root meta

private:
    std::string ObjectName;
};
