#pragma once
#include "Actor.h"

class Player : public Actor {
    QCLASS()
    REFLECTION_BODY(Player, Actor)
public:
    QPROPERTY() int   Ammo = 30;
    QPROPERTY() float Zoom = 1.25f;
};
