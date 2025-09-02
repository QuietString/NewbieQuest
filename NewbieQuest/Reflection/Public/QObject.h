#pragma once
#include <string>
#include "ClassInfo.h"

namespace qreflect {

    class QObject {
        public:
        virtual ~QObject() = default;

        const std::string& GetObjectName() const { return ObjectName; }
        void SetObjectName(std::string n) { ObjectName = std::move(n); }

        virtual ClassInfo& GetClassInfo() const = 0;
        static ClassInfo& StaticClass(); // root meta

    private:
        std::string ObjectName;
    };

}