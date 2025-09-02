#pragma once
#include "../Reflection/Public/Macros.h"

class Actor : public qreflect::QObject {
    QCLASS()
    REFLECTION_BODY(Actor, qreflect::QObject)
public:
    QPROPERTY() int   Health = 100;
    QPROPERTY() float Speed  = 4.5f;
    QPROPERTY() bool  bAlive = true;
};
