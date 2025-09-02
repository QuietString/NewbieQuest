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

    // Properties
    std::vector<std::unique_ptr<PropertyBase>> Props;

    std::function<std::unique_ptr<QObject>()> Factory;
        
    template <typename Fn>
    void ForEachProperty(const Fn& fn) const {
        if (Base) Base->ForEachProperty(fn);
        for (auto& p : Props) fn(*p);
    }
};

struct Registry {
    std::unordered_map<std::string, ClassInfo*> Classes;

    static Registry& Get() { static Registry R; return R; }
    void Register(ClassInfo* ci) { Classes[ci->Name] = ci; }
    ClassInfo* Find(const std::string& name) {
        auto it = Classes.find(name);
        return it==Classes.end()? nullptr : it->second;
    }
};

struct StructInfo {
    std::string Name;
    std::vector<std::unique_ptr<PropertyBase>> Props;

    template <typename Fn>
    void ForEachProperty(const Fn& fn) const {
        for (auto& p : Props) fn(*p);
    }
};
    
struct StructRegistry {
    std::unordered_map<std::string, StructInfo*> Structs;

    static StructRegistry& Get() { static StructRegistry R; return R; }
    void Register(StructInfo* si) { Structs[si->Name] = si; }
    StructInfo* Find(const std::string& name) {
        auto it = Structs.find(name);
        return it==Structs.end()? nullptr : it->second;
    }
};