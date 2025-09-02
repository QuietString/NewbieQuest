#include "Assets.h"
#include "Serialization.h"
#include <utility>

#include "Binary.h"

namespace qreflect::assets {

    namespace fs = std::filesystem;

    static fs::path& _root_storage() {
        static fs::path root;
        return root;
    }

    void SetAssetRoot(const fs::path& p) { _root_storage() = p; }

    const fs::path& EnsureAssetRoot() {
        auto& r = _root_storage();
        if (r.empty()) r = fs::current_path() / "Contents";
        fs::create_directories(r);
        return r;
    }

    fs::path MakeAssetPath(std::string name) {
        if (name.size() < 7 || name.substr(name.size() - 7) != ".qasset")
            name += ".qasset";
        fs::path full = EnsureAssetRoot() / name;
        fs::create_directories(full.parent_path());
        return full;
    }

    bool SaveAsset(const QObject& obj, std::string name) {
        fs::path full = MakeAssetPath(std::move(name));
        return SaveQAsset(obj, full.string());
    }

    std::unique_ptr<QObject> LoadAsset(std::string name) {
        fs::path full = MakeAssetPath(std::move(name));
        return LoadQAsset(full.string());
    }

    bool SaveAssetBinary(const QObject& obj, const std::string name)
    {
        auto full = MakeAssetPath(std::move(name));
        full.replace_extension(".qassetb");
        return SaveQAssetBinary(obj, full.string());
    }

    std::unique_ptr<QObject> LoadAssetBinary(const std::string name)
    {
        auto full = MakeAssetPath(std::move(name));
        full.replace_extension(".qassetb");
        return LoadQAssetBinary(full.string());
    }
}
