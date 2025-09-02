#pragma once
#include <memory>
#include <string>
#include <type_traits>
#include "TypeTraits.h"

namespace qreflect {

struct StructInfo; // fwd

struct PropertyBase {
    std::string Name;
    std::string TypeName;
    BasicKind   Kind;

    PropertyBase(std::string n, std::string tn, BasicKind k)
        : Name(std::move(n)), TypeName(std::move(tn)), Kind(k) {}
    virtual ~PropertyBase() = default;

    virtual void*       Ptr(void* obj) const = 0;
    virtual const void* CPtr(const void* obj) const = 0;

    virtual std::string GetAsString(const void* obj) const = 0;
    virtual bool        SetFromString(void* obj, const std::string& s) const = 0;

    // 구조체면 StructInfo 제공, 아니면 nullptr
    virtual const StructInfo* GetStructInfo() const { return nullptr; }
};

// -------- 원시 타입 프로퍼티 --------
template <typename Owner, typename T>
struct TypedProperty : PropertyBase {
    T Owner::* Member;
    explicit TypedProperty(const char* name, T Owner::* m)
        : PropertyBase(name, TypeTraits<T>::Name(), 
                       std::is_same_v<T,bool>  ? BasicKind::Bool  :
                       std::is_same_v<T,int>   ? BasicKind::Int   :
                                                 BasicKind::Float),
          Member(m) {}

    void* Ptr(void* obj) const override { return &(static_cast<Owner*>(obj)->*Member); }
    const void* CPtr(const void* obj) const override { return &(static_cast<const Owner*>(obj)->*Member); }

    std::string GetAsString(const void* obj) const override {
        const T& v = *static_cast<const T*>(CPtr(obj));
        return to_string(v);
    }
    bool SetFromString(void* obj, const std::string& s) const override {
        T& v = *static_cast<T*>(Ptr(obj));
        return from_string(s, v);
    }
};

// QSTRUCT 감지
template <typename T, typename = void>
struct IsReflectStruct : std::false_type {};
template <typename T>
struct IsReflectStruct<T, std::void_t<decltype(T::StaticStruct())>> : std::true_type {};

// -------- 구조체 프로퍼티 --------
template <typename Owner, typename T>
struct TypedStructProperty : PropertyBase {
    static_assert(IsReflectStruct<T>::value, "T must be a QSTRUCT type");
    T Owner::* Member;
    const StructInfo* SI;

    explicit TypedStructProperty(const char* name, T Owner::* m)
        : PropertyBase(name, T::StaticStruct().Name, BasicKind::Struct),
          Member(m), SI(&T::StaticStruct()) {}

    void* Ptr(void* obj) const override { return &(static_cast<Owner*>(obj)->*Member); }
    const void* CPtr(const void* obj) const override { return &(static_cast<const Owner*>(obj)->*Member); }

    // 구조체 자체를 문자열로 바꾸지는 않음(leaf만 사용)
    std::string GetAsString(const void*) const override { return "<struct>"; }
    bool        SetFromString(void*, const std::string&) const override { return false; }

    const StructInfo* GetStructInfo() const override { return SI; }
};

// -------- MakeProperty: 원시 or 구조체 자동 분기 --------
template <typename Owner, typename T>
inline std::unique_ptr<PropertyBase> MakeProperty(const char* name, T Owner::* member) {
    if constexpr (IsReflectStruct<T>::value) {
        return std::make_unique<TypedStructProperty<Owner, T>>(name, member);
    } else {
        static_assert(std::is_same_v<T,bool> || std::is_same_v<T,int> || std::is_same_v<T,float>,
                      "Only bool/int/float or QSTRUCT are supported.");
        return std::make_unique<TypedProperty<Owner, T>>(name, member);
    }
}

} // namespace qreflect
