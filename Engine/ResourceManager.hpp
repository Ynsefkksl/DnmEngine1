#pragma once

#include "Graphics/Mesh.hpp"
#include "Graphics/Texture.hpp"
#include "Graphics/Material.hpp"
#include "Graphics/Sampler.hpp"

#include <string>
#include <unordered_map>

//forward declarations
namespace fastgltf {
    class Mesh;
    class Material;
    class Image;
    class Primitive;
    class Asset;
    class Node;
}

//TODO: GLTF support will be removed

class ENGINE_API ResourceManager {
public:
    ResourceManager();
    ~ResourceManager() = default;

    void LoadResource(const Scene& scene);

    //[[nodiscard]] std::shared_ptr<Mesh<>> GetMesh(const std::string& name) { auto it = m_meshes.find(name); return (it != m_meshes.end()) ? it->second : nullptr; }
    [[nodiscard]] Material GetMaterial(const std::string& name) { auto it = m_materials.find(name); return (it != m_materials.end()) ? it->second : Material(); }
    [[nodiscard]] Texture GetTexture(const std::string& name) { auto it = m_textures.find(name); return (it != m_textures.end()) ? it->second : Texture(); }

    //[[nodiscard]] FORCE_INLINE const std::unordered_map<std::string, std::shared_ptr<Mesh<>>>& GetMeshes() const noexcept { return m_meshes; }
    [[nodiscard]] FORCE_INLINE const std::unordered_map<std::string, Texture>& GetTextures() const noexcept { return m_textures; }
    [[nodiscard]] FORCE_INLINE const std::unordered_map<std::string, Material>& GetMaterials() const noexcept { return m_materials; }

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager(ResourceManager&&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    ResourceManager& operator=(ResourceManager&&) = delete;
private:
    std::unordered_map<std::string, std::shared_ptr<Mesh<DefualtVertex, DefaultInstanceData>>> m_meshes;
    std::unordered_map<std::string, Texture> m_textures;
    std::unordered_map<std::string, Material> m_materials;

    //temp
    std::shared_ptr<Image> m_envMap;
    std::shared_ptr<Image> m_irradianceMap;
    std::shared_ptr<Image> m_prefilterMap;
    std::shared_ptr<Image> m_lutTexture;
    std::unique_ptr<Sampler> m_iblSampler;
    //temp

    MaterialBuffer* m_materialBuffer = nullptr;
    std::shared_ptr<RawBuffer> m_materialResourceBuffer = nullptr;

    void LoadImage(const std::string& name, ImageFormat format);
    void LoadEnvMap(const std::string& name);
    void LoadAssetGltf(const std::string& name);
    void LoadMeshGltf(const fastgltf::Mesh& meshGltf, const fastgltf::Asset& asset);
    void LoadImageGltf(const fastgltf::Image& imageGltf, const fastgltf::Asset& asset, ImageFormat format);
    void LoadMaterialGltf(const fastgltf::Material& material, const fastgltf::Asset& asset);
    void LoadNodeGltf(const fastgltf::Node& node, const fastgltf::Asset& asset, Scene* scene);
    //for mesh
    static std::vector<DefualtVertex> LoadVertices(const fastgltf::Primitive& primitive, const fastgltf::Asset& asset);
    static std::vector<uint32_t> LoadIndices(const fastgltf::Primitive& primitive, const fastgltf::Asset& asset);

    friend class Engine;
};