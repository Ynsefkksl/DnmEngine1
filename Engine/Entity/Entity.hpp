#pragma once

#include "Utility/EngineDefines.hpp"
#include "Entity/Component.hpp"

#include <string>
#include <vector>
#include <string_view>

class Script : public Component {
public:
    explicit Script(const Entity* entity) : Component(entity) {}
    ~Script() override = default;
    
    virtual void Init() = 0;
    virtual void Update() = 0;
    virtual void CleanUp() = 0;
};

#define SCRIPT_CONSTRUCTOR using Script::Script

class ENGINE_API Entity {
    Entity(const std::string_view name, const Entity* parent, const class Scene* scene) : parent(parent), scene(scene), name(name) {}
public:
    Entity(Entity&& other) noexcept : parent(nullptr), scene(nullptr), m_components(std::move(other.m_components)),
                                      m_children(std::move(other.m_children)), name(std::move(other.name)) {}

    template <typename T, typename ...Args>
    requires std::is_constructible_v<T, Args...> && std::is_base_of_v<Component, T>
    T* AddComponent(Args... args) {
        return static_cast<T*>(m_components.emplace_back(new T(std::forward<Args>(args)...)));
    }

    template <typename T>
    requires std::is_base_of_v<Component, T>
    T* AddComponent(T* component) {
        return static_cast<T*>(m_components.emplace_back(component));
    }

    template <typename T>
    requires std::is_base_of_v<Component, T>
    T* GetComponent() const {
        for (auto* component : m_components)
            if (auto* derived = dynamic_cast<T*>(component))
                return derived;
        return nullptr;
    }

    Entity& CreateChild(const std::string& childName) {
        Entity entity(childName, this, scene);
        return m_children.emplace_back(std::move(entity));
    }

    [[nodiscard]] const std::vector<Entity>& GetChildren() const { return m_children; }

    [[nodiscard]] std::string_view GetName() const { return name; }

    const Entity* parent;
    const Scene* scene;

    Entity(const Entity&) = delete;
    Entity& operator=(const Entity&) = delete;
    Entity& operator=(Entity&&) = default;
private:    
    std::vector<Component*> m_components;
    std::vector<Entity> m_children;

    std::string name;

    friend Scene;
    friend class Engine;
};