#include "Object.h"

qreflect::ClassInfo& QObject::StaticClass() {
    static qreflect::ClassInfo ci;
    static bool inited = false;
    if (!inited) {
        ci.Name = "QObject";
        ci.Base = nullptr;
        ci.Factory = nullptr; // root is abstract-like; not instantiable
        qreflect::Registry::Get().Register(&ci);
        inited = true;
    }
    return ci;
}