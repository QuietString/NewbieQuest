#pragma once
#include "../Reflection/Public/Macros.h"

struct FVector {
    QSTRUCT()
    REFLECTION_STRUCT_BODY(FVector)
public:
    float X = 0.f;
    float Y = 0.f;
    float Z = 0.f;
};
