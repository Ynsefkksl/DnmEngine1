#pragma once

#include "Entity/Component.hpp"
#include "Entity/Transform.hpp"
#include "Graphics/Camera.hpp"
#include "Graphics/Mesh.hpp"
#include "Graphics/Image.hpp"
#include "Graphics/RawBuffer.hpp"
#include "Graphics/Light.hpp"
#include "Graphics/Swapchain.hpp"
#include "Graphics/Pipeline.hpp"
#include "Graphics/RenderPass.hpp"
#include "Graphics/Descriptor.hpp"
#include <memory>
#include <string>
#include <string_view>
#include <vector>

constexpr uint32_t TextureUnitCount = 128;
constexpr DescriptorLocation TextureUnitDstLocation = {0, 1};

class Scene;

class ENGINE_API Renderer {
public:
    Renderer() = default;
    ~Renderer();

    // Prevent resource access to the renderer before it is initialized
    void CreateResources();

    void Render();

    [[nodiscard]] FORCE_INLINE Texture GetTexture(const uint32_t index) const {
        if (index >= TextureUnitCount)
            LogError("Texture index out of bounds");
        return m_textures[index];
    }

    Texture AddTexture(const std::shared_ptr<Image>& image, Sampler* sampler) {
        if (m_textures.size() >= TextureUnitCount)
            LogError("Texture index out of bounds");

        auto texture =  m_textures.emplace_back(Texture{image, sampler, static_cast<uint32_t>(m_textures.size())});

        GetGeometryPassPipelineLayout()->GetDescriptor({0, 1})->SetResource( {
            ImageResource{
                .subresource = ImageSubResource {
                       .mipmapLevelCount = image->GetMipmapCount()
                   },
               .image = image.get(),
               .sampler = GetSampler(),
               }},
                m_textures.size() - 1);

        return texture;
    }

    uint32_t AddShadowMap(ShadowMap& light) {
        shadow_maps.emplace_back(&light);

        GetLightingPassPipelineLayout()
            ->GetDescriptor({0, 5})->SetResource(
            {ImageResource{
                .subresource = {.baseMipmapLevel = 1},
                .image = light.image.get(),
                .sampler = GetSampler()}},
                shadow_maps.size() - 1
        );



        shadowBlurPipeline->GetPipelineLayout()->GetDescriptor({0, 0})->SetResource(
            {
                ImageResource{
                    .subresource = ImageSubResource {
                        .baseMipmapLevel = 0,
                        .mipmapLevelCount = 1
                    },
                    .image = light.image.get()
                },
                ImageResource{
                    .subresource = ImageSubResource {
                        .baseMipmapLevel = 1,
                        .mipmapLevelCount = 1
                    },
                    .image = light.image.get()
                },
            }, 0);

        return shadow_maps.size() - 1;
    }

    [[nodiscard]] FORCE_INLINE std::shared_ptr<Image> GetPlaceholderImage() const { return placeholderImage; }

    [[nodiscard]] FORCE_INLINE std::shared_ptr<Swapchain> GetSwapChain() const { return m_swapchain; }
    
    [[nodiscard]] LightHandle CreateLight(const SpotLightData& data) const;
    [[nodiscard]] LightHandle CreateLight(const DirectionalLightData& data) const;
    [[nodiscard]] LightHandle CreateLight(const PointLightData& data) const;

    [[nodiscard]] FORCE_INLINE std::shared_ptr<RawBuffer> GetLightResourceBuffer() const { return m_lightResourceBuffer; }
    
    [[nodiscard]] FORCE_INLINE LightBuffer& GetLightDatas() const { return *m_lightBuffer; }
    [[nodiscard]] FORCE_INLINE SpotLightData& GetSpotLightData(const LightHandle handle) const { return m_lightBuffer->spotLightArray[handle]; }
    [[nodiscard]] FORCE_INLINE DirectionalLightData& GetDirectionalLightData(LightHandle handle) const { return m_lightBuffer->directionalLightArray[handle]; }
    [[nodiscard]] FORCE_INLINE PointLightData& GetPointLightData(const LightHandle handle) const { return m_lightBuffer->pointLightArray[handle]; }
    
    [[nodiscard]] FORCE_INLINE Sampler* GetSampler() const { return m_sampler; }
    [[nodiscard]] FORCE_INLINE Sampler* GetShadowSampler() const { return m_depthSampler; }

    [[nodiscard]] FORCE_INLINE std::shared_ptr<Camera> GetCurrentCamera() const { return currentCamera; }
    FORCE_INLINE void SetCurrentCamera(const std::shared_ptr<Camera>& camera) { currentCamera = camera; }

    [[nodiscard]] FORCE_INLINE std::shared_ptr<Pipeline> GetShadowPassPipeline() const { return shadowPassPipeline; }
    [[nodiscard]] FORCE_INLINE std::shared_ptr<Pipeline> GetGeometryPassPipeline() const { return geometryPassPipeline; }
    [[nodiscard]] FORCE_INLINE std::shared_ptr<Pipeline> GetLightningPassPipeline() const { return lightingPassPipeline; }
    [[nodiscard]] FORCE_INLINE std::shared_ptr<Pipeline> GetEnvMapPipeline() const { return envMapPipeline; }

    [[nodiscard]] FORCE_INLINE std::shared_ptr<PipelineLayout> GetShadowPassPipelineLayout() const { return shadowPassPipeline->GetPipelineLayout(); }
    [[nodiscard]] FORCE_INLINE std::shared_ptr<PipelineLayout> GetGeometryPassPipelineLayout() const { return geometryPassPipeline->GetPipelineLayout(); }
    [[nodiscard]] FORCE_INLINE std::shared_ptr<PipelineLayout> GetLightingPassPipelineLayout() const { return lightingPassPipeline->GetPipelineLayout(); }
    [[nodiscard]] FORCE_INLINE std::shared_ptr<PipelineLayout> GetEnvMapPipelineLayout() const { return envMapPipeline->GetPipelineLayout(); }

    [[nodiscard]] FORCE_INLINE MeshManager<DefualtVertex, DefaultInstanceData>& GetMeshManager() { return m_mesh_manager; }

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;
    Renderer(Renderer &&) = delete;
    Renderer &operator=(Renderer &&) = delete;
private:
    void InitImGui() const;
    void RecordRenderCommandBuffers();

    MeshManager<DefualtVertex, DefaultInstanceData> m_mesh_manager;

    //to access these Textures with handle
    std::vector<Texture> m_textures;
    std::vector<ShadowMap*> shadow_maps;
    DirectionalLight* directionalLight{};
    glm::vec3 dir = {0.2, -0.5, 0.7};
    float intensity = {3};
    float distance = {300};

    std::shared_ptr<RawBuffer> cameraBuffer;
    CameraGPU* cameraPtr = nullptr;
    glm::vec4* cam_pos = nullptr;

    std::shared_ptr<GraphicsPipeline> shadowPassPipeline;
    std::shared_ptr<ComputePipeline> shadowBlurPipeline;
    std::shared_ptr<GraphicsPipeline> geometryPassPipeline;
    std::shared_ptr<GraphicsPipeline> lightingPassPipeline;
    std::shared_ptr<GraphicsPipeline> envMapPipeline;

    std::shared_ptr<Image> gBuffer;

    Sampler* m_sampler{};
    Sampler* m_depthSampler{};

    std::shared_ptr<Camera> currentCamera;
    std::shared_ptr<Image> placeholderImage;
    std::shared_ptr<Swapchain> m_swapchain;
    std::shared_ptr<RawBuffer> cam_pos_buffer;
    
    RenderPass shadowPass{};
    RenderPass geometryPass{};
    RenderPass lightingPass{};
    std::shared_ptr<Image> depthImage;
    std::shared_ptr<Image> shadowDepthImage;

    std::vector<CommandBuffer> render_command_buffers{};
    std::unique_ptr<CommandBuffer> ui_command_buffer{};
    uint32_t currentFrame = 0;

    vk::raii::QueryPool timeStrapQueryPool{nullptr};
    double geometryPassDuration{};
    double lightingPassDuration{};
    double shadowPassDuration{};
    float timestampPeriod{};

    vk::raii::QueryPool shadowPassQueryPool{nullptr};
    uint64_t shadowPassDrawedPrimativeCount{};
    uint64_t shadowPassVertexShaderInvocations{};

    vk::raii::QueryPool lightingPassQueryPool{nullptr};
    uint64_t lightingFragmentShaderInvocations{};

    vk::raii::QueryPool geometryPassQueryPool{nullptr};
    uint64_t drawedPrimativeCount{};
    uint64_t vertexShaderInvocations{};
    uint64_t fragmentShaderInvocations{};
    uint64_t clippingInvocations{};

    std::shared_ptr<RawBuffer> m_lightResourceBuffer;
    LightBuffer* m_lightBuffer{};

    friend Engine;
    friend Scene;
};