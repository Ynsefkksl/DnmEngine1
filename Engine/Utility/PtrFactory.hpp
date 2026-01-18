#pragma once

#include <memory>
#include <type_traits>

template<typename T>
class PtrFactory {
public:
    template<typename ...Args>
    requires std::is_constructible_v<T, Args...>
    static std::shared_ptr<T> New(Args&&... args) { return std::make_shared<T>(args...); }

    template<typename ...Args>
    requires std::is_constructible_v<T, Args...>
    static std::shared_ptr<T> NewShared(Args&&... args) { return std::make_shared<T>(args...); }

    template<typename ...Args>
    requires std::is_constructible_v<T, Args...>
    static std::unique_ptr<T> NewUnique(Args&&... args) { return std::make_unique<T>(args...); }
};