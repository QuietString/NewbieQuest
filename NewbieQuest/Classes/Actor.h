#pragma once
#include "Object.h"
#include "Reflection/Public/Macros.h"

class Actor : public QObject {
    QCLASS()
    REFLECTION_BODY(Actor, QObject)
public:
    QPROPERTY() int   Health = 100;
    QPROPERTY() float Speed  = 4.5f;
    QPROPERTY() bool  bAlive = true;
};
