#pragma once
#include <memory>
#include <string>
#include <concepts>

#include "TypeTraits.h"

struct StructInfo;

// primitive + struct
enum class BasicKind : std::uint8_t;

struct PropertyBase
{
    std::string Name;
    std::string TypeName;
    BasicKind   Kind;

    PropertyBase(std::string InPropertyName, std::string TypeName, BasicKind k)
        : Name(std::move(InPropertyName)), TypeName(std::move(TypeName)), Kind(k) {}
    virtual ~PropertyBase() {}

    virtual void*       Ptr(void* Obj) const = 0;
    virtual const void* ConstPtr(const void* Obj) const = 0;

    virtual std::string GetAsString(const void* Obj) const = 0;
    virtual bool        SetFromString(void* Obj, const std::string& s) const = 0;

    // provide StructInfo if struct, else nullptr
    virtual const StructInfo* GetStructInfo() const { return nullptr; }
};

// -------- primitive type property --------
template <typename Owner, typename T>
struct TypedProperty : PropertyBase {

    // A pointer-to-member to associated property 
    T Owner::* MemberPtr;
    
    explicit TypedProperty(const char* InPropertyName, T Owner::* InMemberPtr)
        : PropertyBase(InPropertyName, TypeTraits<T>::Name(), 
                       std::is_same_v<T,bool>  ? BasicKind::Bool  :
                       std::is_same_v<T,int>   ? BasicKind::Int   :
                                                 BasicKind::Float),
          MemberPtr(InMemberPtr)
    {}

    void* Ptr(void* Obj) const override
    {
        return &(static_cast<Owner*>(Obj)->*MemberPtr);
    }
    
    const void* ConstPtr(const void* Obj) const override
    {
        return &(static_cast<const Owner*>(Obj)->*MemberPtr);
    }

    std::string GetAsString(const void* Obj) const override {
        const T& v = *static_cast<const T*>(ConstPtr(Obj));
        return ToString(v);
    }
    bool SetFromString(void* Obj, const std::string& s) const override {
        T& v = *static_cast<T*>(Ptr(Obj));
        return FromString(s, v);
    }
};

// Check a property is struct-based
template<class T>
concept ReflectStruct = requires
{
    { T::StaticStruct() } -> std::convertible_to<const StructInfo&>;
};

template<class T>
struct IsReflectStruct : std::bool_constant<ReflectStruct<T>> {};

// --------  Struct Property --------
template <typename Owner, typename T>
struct TypedStructProperty : PropertyBase {
    
    static_assert(IsReflectStruct<T>::value, "T must be a QSTRUCT type");

    T Owner::* MemberPtr;
    
    const StructInfo* SI;

    explicit TypedStructProperty(const char* InPropertyName, T Owner::* InMemberPtr)
        : PropertyBase(InPropertyName, T::StaticStruct().Name, BasicKind::Struct),
          MemberPtr(InMemberPtr), SI(&T::StaticStruct()) {}

    void* Ptr(void* Obj) const override { return &(static_cast<Owner*>(Obj)->*MemberPtr); }
    const void* ConstPtr(const void* Obj) const override { return &(static_cast<const Owner*>(Obj)->*MemberPtr); }

    //  doesn't change struct to literal string. (only use leaf)
    std::string GetAsString(const void*) const override { return "<struct>"; }
    bool        SetFromString(void*, const std::string&) const override { return false; }

    const StructInfo* GetStructInfo() const override { return SI; }
};

// Owner: type of owning class
// T: decltype(ThisClass::Member)
// MemberPtr: pointer to data member, not normal raw pointer.
// IsReflectStruct to check the new property is a struct type or not.  
template <typename Owner, typename T>
inline std::unique_ptr<PropertyBase> MakeProperty(const char* PropertyName, T Owner::* MemberPtr)
{
    if constexpr (IsReflectStruct<T>::value) // T is QStruct type
    {
        return std::make_unique<TypedStructProperty<Owner, T>>(PropertyName, MemberPtr);
    }
    else // T is a primitive or non-supported type
    {
        static_assert(std::is_same_v<T,bool> || std::is_same_v<T,int>|| std::is_same_v<T,float>, "Only bool/int/float or QSTRUCT are supported.");
        return std::make_unique<TypedProperty<Owner, T>>(PropertyName, MemberPtr);
    }
}