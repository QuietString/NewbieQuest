#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

// Property.hpp를 포함해야 하지만, 순환 의존을 피하려면
// 여기서는 forward-declare 후 소스/다른 헤더에서 include 합니다.
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
