#pragma once

#include <filesystem>

typedef __int64 (*FARPROC)() __attribute__((stdcall));
FARPROC LoadShared(const std::filesystem::path& path, const std::string& loaderFunc);

template <typename ReturnType>
ReturnType LoadShared(const std::filesystem::path path, const std::string loaderFunc) {
    return (ReturnType)LoadShared(path, loaderFunc);
}