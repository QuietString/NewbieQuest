#pragma once
#include "Reflection/Public/Macros.h"

#include "Object.h"
#include "Vector.h"

class QMonster : public QObject
{
    REFLECTION_BODY(QMonster, QObject)

private:
    int Level = 10;
    float Rage = 0.25f;
    bool bBoss = false;

    FVector Position;
public:
    int GetLevel() const { return Level; }
    float GetRage()  const { return Rage;  }
    bool IsBoss() const { return bBoss; }

    void SetLevel(int v) { Level = v; }
    void SetRage(float v) { Rage  = v; }
    void SetBoss(bool v) { bBoss = v; }

    FVector& GetPosition() { return Position; }
    const FVector& GetPositionRef() const { return Position; }
};
