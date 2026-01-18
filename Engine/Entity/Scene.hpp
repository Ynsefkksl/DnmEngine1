#pragma once

#include "Entity/Entity.hpp"
#include "ResourceManager.hpp"
#include <string>
#include <string_view>
#include <vector>

class ENGINE_API Scene {
public:
    Scene(std::string_view name);
    
    [[nodiscard]] Entity& CreateEntity(std::string_view name);

    [[nodiscard]] Script* GetScript(const std::string& name, Entity* entity);

    [[nodiscard]] const auto& GetUsedShaders() const noexcept { return m_usedShaders; }
    [[nodiscard]] const auto& GetUsedMaterials() const noexcept { return m_usedMaterials; }
    [[nodiscard]] const auto& GetUsedImages() const noexcept { return m_usedImages; }
    [[nodiscard]] const auto& GetUsedAssets() const noexcept { return m_usedAssets; }
    [[nodiscard]] const std::string& GetUsedEnvMap() const noexcept { return m_usedEnvMap; }
    [[nodiscard]] const auto& GetEntities() const noexcept { return m_entities; }

    Scene(const Scene &) = delete;
    Scene &operator=(const Scene &) = delete;
    Scene(Scene &&) = delete;
    Scene &operator=(Scene &&) = delete;
private:
    std::vector<std::string> m_usedShaders;
    std::vector<std::string> m_usedMaterials;
    std::vector<std::string> m_usedImages;
    std::vector<std::string> m_usedAssets;
    std::string m_usedEnvMap;

    std::unordered_map<std::string, Entity> m_entities;
    std::vector<Script*> m_scripts;

    void RunScripts_initialize() {
        for (auto* script : m_scripts)
            script->Init();
    }

    void RunScripts_update() {
        for (auto* script : m_scripts)
            script->Update();
    }

    void RunScripts_cleanUp() {
        for (auto* script : m_scripts)
            script->CleanUp();
    }
    
    void LoadScriptFuncs(std::vector<std::string>& name);
        
    friend class Engine;
};