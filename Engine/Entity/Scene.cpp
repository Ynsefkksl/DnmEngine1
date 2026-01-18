#include "Entity/Scene.hpp"

#include "Engine.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/Image.hpp"
#include "Utility/EngineDefines.hpp"

#include <glm/gtc/quaternion.hpp>

#include <cstddef>

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

Scene::Scene(std::string_view name) {
    std::filesystem::path path("./Scenes/");
    path += name;
    path += ".json";

    std::ifstream file(path);
    auto data = nlohmann::json::parse(file);

    //used assets
    {
        auto usedAssets = data["assets"];

        for (auto& asset : usedAssets)
            m_usedAssets.emplace_back(asset);
    }

    //used materials
    {
        auto usedMaterials = data["materials"];

        for (auto& material : usedMaterials)
            m_usedMaterials.emplace_back(material);
    }

    // used Textures
    {
        auto usedTextures = data["textures"];

        for (auto& texture : usedTextures)
            m_usedImages.emplace_back(texture);
    }

    //get env map
    {
        auto envMap = data["environmentMap"];
        m_usedEnvMap = envMap["name"];
    }

    // create scripts
    {
        auto usedScripts = data["scripts"];

        for (auto& scriptName : usedScripts) {
            auto& entity = CreateEntity(scriptName);
            auto* script = GetScript(scriptName, &entity);

            if (script == nullptr)
                LogWarning("{} not found", std::string(scriptName));

            entity.AddComponent<Script>(script);
        }
    }
}

[[nodiscard]] Entity& Scene::CreateEntity(std::string_view name) {
    uint32_t count = 0;

    while (count++ < 1024) {
        bool isExist = m_entities.find(std::format("{} ({})", name, count)) != m_entities.end();

        if (!isExist) {
            Entity entity(name, nullptr, this);
            return m_entities.emplace(std::string(name), std::move(entity)).first->second;
        }
    }

    LogError("too many entities have the same name., name: {}", name);
    return m_entities.end()->second;
}

[[nodiscard]] Script* Scene::GetScript(const std::string& name, Entity* entity) {
    auto scriptLoader = Engine::Get().GetScriptLoader(name);
    if (scriptLoader == nullptr)
        return nullptr;

    return m_scripts.emplace_back(scriptLoader(entity));
}