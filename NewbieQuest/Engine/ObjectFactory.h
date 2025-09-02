#pragma once
#include <memory>
#include <utility>
#include <string>
#include "Object.h"

// Create an instance by new and set ObjectName
template <typename T, typename... Args>
std::unique_ptr<T> NewObject(std::string name, Args&&... args) {
    static_assert(std::is_base_of_v<QObject, T>, "T must derive from QObject");
    auto obj = std::make_unique<T>(std::forward<Args>(args)...);
    obj->SetObjectName(std::move(name));
    return obj;
}
