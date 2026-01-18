#include "Utility/LoadShared.hpp"

#include <Windows.h>
#include "Log.hpp"

FARPROC LoadShared(const std::filesystem::path& path, const std::string& loaderFunc) {
    if (std::filesystem::exists(path)) {
        std::string error = path.string();
        error += " don't found";
        LogError("{}", error);
    }

    std::string nameShared = path.filename().string();

    if (const HMODULE hModule = LoadLibrary(nameShared.c_str())) {
        const auto scriptLoadFunc = GetProcAddress(hModule, loaderFunc.c_str());

        if (scriptLoadFunc == nullptr) {
            std::string error = nameShared;
            error += " can't get loader func";
            LogError("{}", error);
        }

        LogInfo("{} is loaded", nameShared);

        return scriptLoadFunc;

    }

    std::string error = nameShared;
    error += " can't opened";
    LogError("{}", error);
    return nullptr;
}
