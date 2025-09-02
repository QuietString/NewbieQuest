#pragma once
#include <string>
#include "Reflection/Public/ClassInfo.h"

class QObject {
public:
    virtual ~QObject() = default;

    const std::string& GetObjectName() const { return ObjectName; }
    void SetObjectName(std::string n) { ObjectName = std::move(n); }

    virtual qreflect::ClassInfo& GetClassInfo() const = 0;
    static qreflect::ClassInfo& StaticClass(); // root meta

private:
    std::string ObjectName;
};