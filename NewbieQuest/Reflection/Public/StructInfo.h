#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

// we need to include Property.h, but to avoid circular dependency
// here we forward-declare and include it from other sources and headers.
namespace qreflect {

    struct PropertyBase; // fwd

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

}
