#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include "Property.h"

class QObject;

struct ClassInfo {
    std::string Name;
    ClassInfo*  Base = nullptr;

    std::vector<std::unique_ptr<PropertyBase>> Properties;

    std::function<std::unique_ptr<QObject>()> Factory;
        
    template <typename Fn>
    void ForEachProperty(const Fn& Func) const {
        if (Base) Base->ForEachProperty(Func);
        for (auto& Property : Properties) Func(*Property);
    }
};

struct Registry {
    std::unordered_map<std::string, ClassInfo*> Classes;

    static Registry& Get() { static Registry R; return R; }
    void Register(ClassInfo* Info) { Classes[Info->Name] = Info; }
    ClassInfo* Find(const std::string& Name) {
        auto It = Classes.find(Name);
        return (It == Classes.end()) ? nullptr : It->second;
    }
};

struct StructInfo {
    std::string Name;
    std::vector<std::unique_ptr<PropertyBase>> Properties;

    template <typename Fn>
    void ForEachProperty(const Fn& Func) const {
        for (auto& Property : Properties) Func(*Property);
    }
};
    
struct StructRegistry {
    std::unordered_map<std::string, StructInfo*> Structs;

    static StructRegistry& Get() { static StructRegistry R; return R; }
    void Register(StructInfo* Info) { Structs[Info->Name] = Info; }
    StructInfo* Find(const std::string& Name) {
        auto It = Structs.find(Name);
        return It==Structs.end()? nullptr : It->second;
    }
};