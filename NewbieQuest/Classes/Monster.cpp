#include "Monster.h"

#include "Vector.h"

BEGIN_REFLECTION(Monster)
    // Since it's a body of Monster's private static member function(_RegisterProperties)
    // it's accessible to private
    QFIELD(Level)
    QFIELD(Rage)
    QFIELD(bBoss)
    QFIELD(Position)
END_REFLECTION()
