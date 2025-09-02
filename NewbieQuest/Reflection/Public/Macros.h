#pragma once
#include <memory>
#include "ClassInfo.h"
#include "Property.h"
#include "Object.h"
#include "StructInfo.h"

#define QCLASS()
#define QPROPERTY()
#define QSTRUCT()

// ----- Class -----
#define REFLECTION_BODY(ClassType, SuperType) \
public: \
    using ThisClass = ClassType; \
    using Super     = SuperType; \
    static ::qreflect::ClassInfo& StaticClass() { \
        static ::qreflect::ClassInfo ci; \
        static bool inited = false; \
        if (!inited) { \
            ci.Name = #ClassType; \
            ci.Base = &SuperType::StaticClass(); \
            ci.Factory = []()->std::unique_ptr<QObject> { return std::make_unique<ClassType>(); }; \
            _RegisterProperties(ci); \
            ::qreflect::Registry::Get().Register(&ci); \
            inited = true; \
        } \
        return ci; \
    } \
    ::qreflect::ClassInfo& GetClassInfo() const override { return StaticClass(); } \
private: \
    static void _RegisterProperties(::qreflect::ClassInfo& ci);

#define BEGIN_REFLECTION(ClassType) \
    void ClassType::_RegisterProperties(::qreflect::ClassInfo& ci) {

#define QFIELD(Member) \
    ci.Props.push_back(::qreflect::MakeProperty<ThisClass>(#Member, &ThisClass::Member));

#define END_REFLECTION() }

// ----- Struct -----
#define REFLECTION_STRUCT_BODY(StructType) \
public: \
    using ThisStruct = StructType; \
    static ::qreflect::StructInfo& StaticStruct() { \
        static ::qreflect::StructInfo si; \
        static bool inited = false; \
        if (!inited) { \
            si.Name = #StructType; \
            _RegisterStructProps(si); \
            ::qreflect::StructRegistry::Get().Register(&si); \
            inited = true; \
        } \
        return si; \
    } \
private: \
    static void _RegisterStructProps(::qreflect::StructInfo& si);

#define BEGIN_REFLECTION_STRUCT(StructType) \
    void StructType::_RegisterStructProps(::qreflect::StructInfo& si) {

#define SFIELD(Member) \
    si.Props.push_back(::qreflect::MakeProperty<ThisStruct>(#Member, &ThisStruct::Member));

#define END_REFLECTION_STRUCT() }
