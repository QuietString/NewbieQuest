#include "Player.h"

BEGIN_REFLECTION(Player)
    // Base(Actor) props are included automatically via ForEachProperty
    QFIELD(Ammo)
    QFIELD(Zoom)
END_REFLECTION()
