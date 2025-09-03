#pragma once
#include "Actor.h"
#include "CoreMinimal.h"

class Player : public Actor
{
    REFLECTION_BODY(Player, Actor)
    
public:
    int Ammo = 30;
    float Zoom = 1.25f;
};
