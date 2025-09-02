#include "../Public/QObject.h"

namespace qreflect {

    ClassInfo& QObject::StaticClass() {
        static ClassInfo ci;
        static bool inited = false;
        if (!inited) {
            ci.Name = "QObject";
            ci.Base = nullptr;
            ci.Factory = nullptr; // root is abstract-like; not instantiable
            Registry::Get().Register(&ci);
            inited = true;
        }
        return ci;
    }
}
