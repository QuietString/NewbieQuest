#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include "ClassInfo.h"
#include "StructInfo.h"
#include "Property.h"
#include "QObject.h"

namespace qreflect {

// recursively walk down to leaf(primitive type) and call back
// emit(name, leafProperty, ownerPtrOfLeaf)
template <typename Emit>
inline void ForEachLeafConst(const QObject& obj, const Emit& emit) {
    std::function<void(const void*, const StructInfo&, const std::string&)> walkStruct;
    walkStruct = [&](const void* structPtr, const StructInfo& si, const std::string& prefix){
        si.ForEachProperty([&](const PropertyBase& sp){
            if (sp.Kind == BasicKind::Struct && sp.GetStructInfo()) {
                const void* nested = sp.CPtr(structPtr);
                walkStruct(nested, *sp.GetStructInfo(), prefix + sp.Name + ".");
            } else {
                emit(prefix + sp.Name, sp, structPtr);
            }
        });
    };

    obj.GetClassInfo().ForEachProperty([&](const PropertyBase& p){
        if (p.Kind == BasicKind::Struct && p.GetStructInfo()) {
            const void* s = p.CPtr(&obj);
            walkStruct(s, *p.GetStructInfo(), p.Name + ".");
        } else {
            emit(p.Name, p, &obj);
        }
    });
}

// non-const version: used for creating setter map
// emit(name, leafProperty, ownerPtrOfLeaf)
template <typename Emit>
inline void ForEachLeaf(QObject& obj, const Emit& emit) {
    std::function<void(void*, const StructInfo&, const std::string&)> walkStruct;
    walkStruct = [&](void* structPtr, const StructInfo& si, const std::string& prefix){
        si.ForEachProperty([&](const PropertyBase& sp){
            if (sp.Kind == BasicKind::Struct && sp.GetStructInfo()) {
                void* nested = sp.Ptr(structPtr);
                walkStruct(nested, *sp.GetStructInfo(), prefix + sp.Name + ".");
            } else {
                emit(prefix + sp.Name, sp, structPtr);
            }
        });
    };

    obj.GetClassInfo().ForEachProperty([&](const PropertyBase& p){
        if (p.Kind == BasicKind::Struct && p.GetStructInfo()) {
            void* s = p.Ptr(&obj);
            walkStruct(s, *p.GetStructInfo(), p.Name + ".");
        } else {
            emit(p.Name, p, &obj);
        }
    });
}

}
