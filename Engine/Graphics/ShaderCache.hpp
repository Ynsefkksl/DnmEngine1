#pragma once

#include "Graphics/Shader.hpp"

class ShaderCache {
public:
    Shader* RegisterShader(std::string_view shaderName) {
        auto [it , _ ] = m_shaders.try_emplace(std::string(shaderName), std::make_unique<Shader>(shaderName));
        return it->second.get();
    }

    [[nodiscard]] Shader* GetShader(const std::string_view shaderName) const {
        const auto it = m_shaders.find(std::string(shaderName));
        return (it == m_shaders.end()) ? nullptr : it->second.get();
    }

    void ClearCache() {
        m_shaders.clear();
    }
private:
    std::unordered_map<std::string, std::unique_ptr<Shader>> m_shaders;
};