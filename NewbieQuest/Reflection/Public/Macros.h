#pragma once
#include <memory>
#include "Property.h"
#include "Object.h"
#include "TypeInfos.h"

// ----- Class -----
#define REFLECTION_BODY(ClassType, SuperType) \
public: \
    using ThisClass = ClassType; \
    using Super     = SuperType; \
    static ::ClassInfo& StaticClass() { \
        static ::ClassInfo CI; \
        static bool Inited = false; \
        if (!Inited) { \
            CI.Name = #ClassType; \
            CI.Base = &SuperType::StaticClass(); \
            CI.Factory = []()->std::unique_ptr<QObject> { return std::make_unique<ClassType>(); }; \
            _RegisterProperties(CI); \
            ::Registry::Get().Register(&CI); \
            Inited = true; \
        } \
        return CI; \
    } \
    ::ClassInfo& GetClassInfo() const override { return StaticClass(); } \
private: \
    static void _RegisterProperties(::ClassInfo& CI);

#define BEGIN_REFLECTION(ClassType) \
    void ClassType::_RegisterProperties(::ClassInfo& CI) {

#define QFIELD(Member) \
    CI.Properties.push_back(::MakeProperty<ThisClass>(#Member, &ThisClass::Member));

#define END_REFLECTION() }

// ----- Struct -----
#define REFLECTION_STRUCT_BODY(StructType) \
public: \
    using ThisStruct = StructType; \
    static ::StructInfo& StaticStruct() { \
        static ::StructInfo si; \
        static bool Inited = false; \
        if (!Inited) { \
            si.Name = #StructType; \
            _RegisterStructProps(si); \
            ::StructRegistry::Get().Register(&si); \
            Inited = true; \
        } \
        return si; \
    } \
private: \
    static void _RegisterStructProps(::StructInfo& si);

#define BEGIN_REFLECTION_STRUCT(StructType) \
    void StructType::_RegisterStructProps(::StructInfo& si) {

#define SFIELD(Member) \
    si.Properties.push_back(::MakeProperty<ThisStruct>(#Member, &ThisStruct::Member));

#define END_REFLECTION_STRUCT() }
