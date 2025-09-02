#pragma once
#include <memory>
#include <string>
#include <filesystem>
#include "QObject.h"

namespace qreflect::assets {

    namespace fs = std::filesystem;

    void SetAssetRoot(const fs::path& p);
    const fs::path& EnsureAssetRoot();         // lazy init: CWD/Contents
    fs::path MakeAssetPath(std::string name);  // append .qasset if missing

    bool SaveAsset(const QObject& obj, std::string name);
    std::unique_ptr<QObject> LoadAsset(std::string name);

    bool SaveAssetBinary(const QObject& obj, const std::string name);
    std::unique_ptr<QObject> LoadAssetBinary(const std::string name);

    template<typename T>
    requires std::is_base_of_v<QObject, T>
    inline bool SaveAssetBinary(const std::unique_ptr<T>& p, const std::string& path)
    {
        return p ? SaveAssetBinary(*p, path) : false;
    }
}
