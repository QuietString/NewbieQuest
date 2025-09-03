#pragma once

#include "CoreMinimal.h"

class Actor : public QObject
{
    REFLECTION_BODY(Actor, QObject)
    
public:
    int Health = 100;
    float Speed = 4.5f;
    bool bAlive = true;
};
