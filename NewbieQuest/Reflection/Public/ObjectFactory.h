#pragma once
#include <memory>
#include <utility>
#include <string>
#include "QObject.h"

namespace qreflect {

    // new 로 객체 만들고 바로 ObjectName까지 지정
    template <typename T, typename... Args>
    std::unique_ptr<T> NewObject(std::string name, Args&&... args) {
        static_assert(std::is_base_of_v<QObject, T>, "T must derive from qreflect::QObject");
        auto obj = std::make_unique<T>(std::forward<Args>(args)...);
        obj->SetObjectName(std::move(name));
        return obj;
    }

}
