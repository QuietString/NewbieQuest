#include "ObjectBase.h"

ClassInfo& QObjectBase::StaticClass() {
    static ClassInfo Ci;
    static bool bInit = false;
    if (!bInit) {
        Ci.Name = "QObject";
        Ci.Base = nullptr;
        Ci.Factory = nullptr;
        Registry::Get().Register(&Ci);
        bInit = true;
    }
    return Ci;
}