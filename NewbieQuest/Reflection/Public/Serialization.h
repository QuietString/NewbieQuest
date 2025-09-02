#pragma once
#include <memory>
#include <string>
#include <ostream>
#include "QObject.h"

namespace qreflect {

    // text .qasset
    bool SaveQAsset(const QObject& obj, const std::string& path);
    std::unique_ptr<QObject> LoadQAsset(const std::string& path);

    // debug dump
    void DumpObject(const QObject& obj, std::ostream& os);

}