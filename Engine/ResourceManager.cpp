#include "ResourceManager.hpp"

#include "Engine.hpp"

#include "Entity/Scene.hpp"
#include "Graphics/RawBuffer.hpp"
#include "Graphics/Mesh.hpp"
#include "Graphics/Renderer.hpp"
#include <glm/gtc/quaternion.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/glm_element_traits.hpp>

#include "Utility/Counter.hpp"

ResourceManager::ResourceManager() {
    // Initialize material buffer
    m_materialResourceBuffer = RawBuffer::NewShared(sizeof(MaterialBuffer), vk::BufferUsageFlagBits::eUniformBuffer, MemoryType::eCpuWrite);
    m_materialBuffer = m_materialResourceBuffer->MapMemory<MaterialBuffer>();
    m_materialBuffer->count = 0;
    std::ranges::fill(m_materialBuffer->array, MaterialData{});

    GetRenderer().GetGeometryPassPipelineLayout()->GetDescriptor({0, 2})->SetResource({m_materialResourceBuffer.get()}, 0);

    m_iblSampler = std::make_unique<Sampler>(vk::Filter::eLinear, vk::SamplerAddressMode::eClampToEdge, vk::CompareOp::eNever);
}

void ResourceManager::LoadImage(const std::string& name, const ImageFormat format) {
    int width, height, channels;
    stbi_uc* data = stbi_load(("./Textures/" + name).c_str(), &width, &height, &channels, 0);
    if (!data) {
        LogError("Failed to load image: {}", name);
        return;
    }

    const auto image = Image::New(ImageCreateInfo{
        {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
        format,
        ImageType::e2D,
        ImageUsageBit::eSampled | ImageUsageBit::eTransferDst,
        MemoryType::eDeviceLocal
    });

    Engine::Get().GetGraphicsThread().SubmitCommands([&] (CommandBuffer& commandBuffer) -> bool {
        commandBuffer.UploadData(*image, {}, data, width * height * static_cast<uint32_t>(channels), 0);
        return true;
    });

    m_textures.try_emplace(name, GetRenderer().AddTexture(image, GetRenderer().GetSampler()));

    stbi_image_free(data);
}

void ResourceManager::LoadEnvMap(const std::string& name) {
    {
        ImageCreateInfo imageCreateInfo{};
        imageCreateInfo.extent = {256, 256, 1};
        imageCreateInfo.format = ImageFormat::eR16G16B16A16Sfloat;
        imageCreateInfo.memoryUsage = MemoryType::eDeviceLocal;
        imageCreateInfo.type = ImageType::eCube;
        imageCreateInfo.arraySize = 6;
        imageCreateInfo.mipmapCount = 5;
        imageCreateInfo.usage = ImageUsageBit::eSampled | ImageUsageBit::eColorAttachment | ImageUsageBit::eTransferSrc | ImageUsageBit::eTransferDst;

        m_envMap = Image::New(imageCreateInfo);
    }
    {
        ImageCreateInfo imageCreateInfo{};
        imageCreateInfo.extent = {32, 32, 1};
        imageCreateInfo.format = ImageFormat::eR16G16B16A16Sfloat;
        imageCreateInfo.memoryUsage = MemoryType::eDeviceLocal;
        imageCreateInfo.type = ImageType::eCube;
        imageCreateInfo.arraySize = 6;
        imageCreateInfo.usage = ImageUsageBit::eSampled | ImageUsageBit::eColorAttachment; 

        m_irradianceMap = Image::New(imageCreateInfo);
    }
    {
        ImageCreateInfo imageCreateInfo{};
        imageCreateInfo.extent = {128, 128, 1};
        imageCreateInfo.format = ImageFormat::eR16G16B16A16Sfloat;
        imageCreateInfo.memoryUsage = MemoryType::eDeviceLocal;
        imageCreateInfo.type = ImageType::eCube;
        imageCreateInfo.arraySize = 6;
        imageCreateInfo.mipmapCount = 5;
        imageCreateInfo.usage = ImageUsageBit::eSampled | ImageUsageBit::eColorAttachment;

        m_prefilterMap = Image::New(imageCreateInfo);
    }
    {
        ImageCreateInfo imageCreateInfo{};
        imageCreateInfo.extent = {512, 512, 1};
        imageCreateInfo.format = ImageFormat::eR32G32Sfloat;
        imageCreateInfo.memoryUsage = MemoryType::eDeviceLocal;
        imageCreateInfo.type = ImageType::e2D;
        imageCreateInfo.arraySize = 1;
        imageCreateInfo.mipmapCount = 1;
        imageCreateInfo.usage = ImageUsageBit::eSampled | ImageUsageBit::eColorAttachment | ImageUsageBit::eTransferSrc | ImageUsageBit::eTransferDst;

        m_lutTexture = Image::New(imageCreateInfo);
    }

    //cube map
    {
        int width, height, nrComponents;
        stbi_set_flip_vertically_on_load(true); 
        float* data = stbi_loadf(("./Assets/" + name).c_str(), &width, &height, &nrComponents, 4);
        stbi_set_flip_vertically_on_load(false); 

        ImageCreateInfo imageCreateInfo{};
        imageCreateInfo.extent = {width, height, 1};
        imageCreateInfo.format = ImageFormat::eR32G32B32A32Sfloat;
        imageCreateInfo.memoryUsage = MemoryType::eDeviceLocal;
        imageCreateInfo.type = ImageType::e2D;
        imageCreateInfo.usage = ImageUsageBit::eSampled | ImageUsageBit::eTransferDst; 

        auto hdrImage = Image::New(imageCreateInfo);

        Engine::Get().GetGraphicsThread().SubmitCommands([&] (CommandBuffer& commandBuffer) -> bool {
            commandBuffer.UploadData(*hdrImage, {}, data, width * height * 16, 0);
            return true;
        });
        Engine::Get().GetGraphicsThread().WaitForFence();

        stbi_image_free(data);

        std::vector<ImageFormat> format {ImageFormat::eR16G16B16A16Sfloat};

        GraphicsPipeline equirectangularToCubeMapPipeline;
        equirectangularToCubeMapPipeline.SetMultisampleStateInfo()
                .SetDynamicStateInfo()
                .SetInputAssamblyStateInfo(vk::PrimitiveTopology::eTriangleList)
                .SetResterStateInfo()
                .SetVertexInputState({}, {})
                .SetDepthStencilState(true, false, vk::CompareOp::eNever)
                .SetViewportState(vk::Viewport(0, 0, static_cast<float>(m_envMap->GetExtent().x), static_cast<float>(m_envMap->GetExtent().y), 0, 1.f),
                                vk::Rect2D({0,0}, vk::Extent2D{m_envMap->GetExtent().x, m_envMap->GetExtent().y}))
                .SetRenderingCreateInfo(format)
                .SetShaderStages("CubeMap.vert", "EquirectangularToCubeMap.frag")
                .CreatePipeline();

        auto pipelineLayout = equirectangularToCubeMapPipeline.GetPipelineLayout();

        pipelineLayout->GetDescriptor({0, 0})
            ->SetResource({ImageResource{.subresource = {}, .image = hdrImage.get(), .sampler = m_iblSampler.get()}}, 0);

        glm::mat4 captureProjection = glm::perspective(glm::radians(90.f), 1.0f, 0.1f, 10.0f);
        std::vector<glm::mat4> captureViews = {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };


        Engine::Get().GetGraphicsThread().SubmitCommands([&] (CommandBuffer& commandBuffer) -> bool {
            commandBuffer.BindPipeline(equirectangularToCubeMapPipeline);

            uint32_t i = 0;
            for (const auto& view : captureViews) {
                ColorAttachmentInfo colorAttachment = m_envMap->GetColorAttachmentInfo(
                    ImageSubResource{
                        .type = ImageResourceType::e2D,
                        .baseMipmapLevel = 0,
                        .baseLayer = i++,
                        .mipmapLevelCount = 1,
                        .layerCount = 1
                    });

                DynamicRenderPassInfo renderPassInfo;
                renderPassInfo.colorAttachments = std::span(&colorAttachment, 1);
                renderPassInfo.extent = {m_envMap->GetExtent().x, m_envMap->GetExtent().y};

                commandBuffer.BeginRenderPass(renderPassInfo);

                glm::mat4 pc = captureProjection * view;

                commandBuffer.PushConstant(
                    *equirectangularToCubeMapPipeline.GetPipelineLayout(), ShaderStageBit::eVertex, &pc, 64);

                commandBuffer.Draw(36, 1);

                commandBuffer.EndRenderPass();
            }

            commandBuffer.GenerateMipMaps(*m_envMap);
            return true;
        });
        Engine::Get().GetGraphicsThread().WaitForFence();
    }

    //irradiance map
    {
        std::vector<ImageFormat> format {ImageFormat::eR16G16B16A16Sfloat};

        GraphicsPipelineCreateInfo pipelineCreateInfo;
        pipelineCreateInfo.vertexShader = "CubeMap.vert";
        pipelineCreateInfo.fragmentShader = "Irradiance.frag";
        pipelineCreateInfo.formats = format;
        pipelineCreateInfo.viewport = vk::Viewport{0, 0, static_cast<float>(m_irradianceMap->GetExtent().x), static_cast<float>(m_irradianceMap->GetExtent().y), 0, 1.f};
        pipelineCreateInfo.extent = vk::Extent2D{m_irradianceMap->GetExtent().x, m_irradianceMap->GetExtent().y};

        GraphicsPipeline irradiancePipeline;
        irradiancePipeline.SetMultisampleStateInfo()
                .SetDynamicStateInfo()
                .SetInputAssamblyStateInfo(vk::PrimitiveTopology::eTriangleList)
                .SetResterStateInfo()
                .SetVertexInputState({}, {})
                //.SetDepthStencilState(true, false, vk::CompareOp::eNever)
                .SetViewportState(vk::Viewport(0, 0, static_cast<float>(m_irradianceMap->GetExtent().x), static_cast<float>(m_irradianceMap->GetExtent().y), 0, 1.f),
                                vk::Rect2D({0,0}, vk::Extent2D{m_irradianceMap->GetExtent().x, m_irradianceMap->GetExtent().y}))
                .SetRenderingCreateInfo(format)
                .SetShaderStages("CubeMap.vert", "Irradiance.frag")
                .CreatePipeline();

        auto pipelineLayout = irradiancePipeline.GetPipelineLayout();

        pipelineLayout->GetDescriptor({0, 0})
            ->SetResource({ImageResource{
                .subresource = {
                    .type = ImageResourceType::eCube,
                    .mipmapLevelCount = 5,
                    .layerCount = 6
                },
            .image = m_envMap.get(),
            .sampler = m_iblSampler.get()}},
            0);

        glm::mat4 captureProjection = glm::perspective(glm::radians(90.f), 1.0f, 0.1f, 10.0f);
        std::vector<glm::mat4> captureViews = {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };

        Engine::Get().GetGraphicsThread().SubmitCommands([&] (CommandBuffer& commandBuffer) -> bool {
            commandBuffer.BindPipeline(irradiancePipeline);

            uint32_t i = 0;
            for (const auto& view : captureViews) {
                ColorAttachmentInfo colorAttachment = m_irradianceMap->GetColorAttachmentInfo(
                    ImageSubResource{
                        .type = ImageResourceType::e2D,
                        .baseMipmapLevel = 0,
                        .baseLayer = i++,
                        .mipmapLevelCount = 1,
                        .layerCount = 1
                        });

                DynamicRenderPassInfo renderPassInfo;
                renderPassInfo.colorAttachments = std::span(&colorAttachment, 1);
                renderPassInfo.extent = {m_irradianceMap->GetExtent().x, m_irradianceMap->GetExtent().y};

                commandBuffer.BeginRenderPass(renderPassInfo);

                glm::mat4 pc = captureProjection * view;

                commandBuffer.PushConstant(
                    *irradiancePipeline.GetPipelineLayout(), ShaderStageBit::eVertex, &pc, 64);

                commandBuffer.Draw(36, 1);

                commandBuffer.EndRenderPass();
            }
            return true;
        });
        Engine::Get().GetGraphicsThread().WaitForFence();
    }

    //prefiltered map
    {
        std::vector<ImageFormat> format {ImageFormat::eR16G16B16A16Sfloat};
        std::vector<vk::DynamicState> dynamicStates {vk::DynamicState::eViewport, vk::DynamicState::eScissor};

        GraphicsPipeline preFilterPipeline;
        preFilterPipeline.SetMultisampleStateInfo()
                        .SetDynamicStateInfo(dynamicStates)
                        .SetInputAssamblyStateInfo(vk::PrimitiveTopology::eTriangleList)
                        .SetResterStateInfo()
                        .SetVertexInputState({}, {})
                        .SetDepthStencilState(true, false, vk::CompareOp::eNever)
                        .SetViewportState(vk::Viewport(0, 0, static_cast<float>(m_prefilterMap->GetExtent().x), static_cast<float>(m_prefilterMap->GetExtent().y), 0, 1.f),
                                        vk::Rect2D({0,0}, vk::Extent2D{m_prefilterMap->GetExtent().x, m_prefilterMap->GetExtent().y}))
                        .SetRenderingCreateInfo(format)
                        .SetShaderStages("CubeMap.vert", "PreFilter.frag")
                        .CreatePipeline();

        auto pipelineLayout = preFilterPipeline.GetPipelineLayout();

        pipelineLayout->GetDescriptor({0, 0})
            ->SetResource({ImageResource{
                .subresource = {
                    .type = ImageResourceType::eCube,
                    .mipmapLevelCount = 5,
                    .layerCount = 6
                },
            .image = m_envMap.get(),
            .sampler = m_iblSampler.get()}},
            0);

        glm::mat4 captureProjection = glm::perspective(glm::radians(90.f), 1.0f, 0.1f, 10.0f);
        std::vector<glm::mat4> captureViews = {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };


        Engine::Get().GetGraphicsThread().SubmitCommands([&] (CommandBuffer& commandBuffer) -> bool {
            commandBuffer.BindPipeline(preFilterPipeline);

            const uint32_t maxMipmapLevel = m_prefilterMap->GetMipmapCount();

            for (uint32_t mipmapLevel = 0; mipmapLevel < maxMipmapLevel; mipmapLevel++) {
                uint32_t mipWidth  = static_cast<uint32_t>(m_prefilterMap->GetExtent().x * std::pow(0.5, mipmapLevel));
                uint32_t mipHeight = static_cast<uint32_t>(m_prefilterMap->GetExtent().y * std::pow(0.5, mipmapLevel));

                commandBuffer.SetViewport({mipWidth, mipHeight});
                commandBuffer.SetScissor({mipWidth, mipHeight});

                uint32_t i = 0;
                for (const auto& view : captureViews) {
                    ColorAttachmentInfo colorAttachment = m_prefilterMap->GetColorAttachmentInfo(
                        ImageSubResource{
                                .type = ImageResourceType::e2D,
                                .baseMipmapLevel = mipmapLevel,
                                .baseLayer = i++,
                                .mipmapLevelCount = 1,
                                .layerCount = 1
                            });

                    DynamicRenderPassInfo renderPassInfo;
                    renderPassInfo.colorAttachments = std::span(&colorAttachment, 1);
                    renderPassInfo.extent = {mipWidth, mipHeight};

                    commandBuffer.BeginRenderPass(renderPassInfo);

                    struct {
                        glm::mat4 mtx;
                        float roughness;
                    } pc {
                        .mtx = captureProjection * view,
                        .roughness = static_cast<float>(mipmapLevel) / static_cast<float>(maxMipmapLevel - 1)
                    };

                    commandBuffer.PushConstant(
                        *preFilterPipeline.GetPipelineLayout(), ShaderStageBit::eVertex | ShaderStageBit::eFragment, &pc, sizeof(pc));

                    commandBuffer.Draw(36, 1);

                    commandBuffer.EndRenderPass();
                }
            }

            return true;
        });
        Engine::Get().GetGraphicsThread().WaitForFence();
    }

    //lut texture
    {
        std::vector<ImageFormat> format {ImageFormat::eR32G32Sfloat};

        GraphicsPipelineCreateInfo pipelineCreateInfo;
        pipelineCreateInfo.vertexShader = "Brdf.vert";
        pipelineCreateInfo.fragmentShader = "Brdf.frag";
        pipelineCreateInfo.formats = format;
        pipelineCreateInfo.viewport = vk::Viewport{0, 0, static_cast<float>(m_lutTexture->GetExtent().x), static_cast<float>(m_lutTexture->GetExtent().y), 0, 1.f};
        pipelineCreateInfo.extent = vk::Extent2D{m_lutTexture->GetExtent().x, m_lutTexture->GetExtent().y};
        pipelineCreateInfo.topology = vk::PrimitiveTopology::eTriangleStrip;

        GraphicsPipeline irradiancePipeline;
        irradiancePipeline.SetMultisampleStateInfo()
                        .SetDynamicStateInfo()
                        .SetInputAssamblyStateInfo(vk::PrimitiveTopology::eTriangleStrip)
                        .SetResterStateInfo()
                        .SetVertexInputState({}, {})
                        .SetDepthStencilState(true, false, vk::CompareOp::eNever)
                        .SetViewportState(vk::Viewport(0, 0, static_cast<float>(m_lutTexture->GetExtent().x), static_cast<float>(m_lutTexture->GetExtent().y), 0, 1.f),
                                        vk::Rect2D({0,0}, vk::Extent2D{m_lutTexture->GetExtent().x, m_lutTexture->GetExtent().y}))
                        .SetRenderingCreateInfo(format)
                        .SetShaderStages("Brdf.vert", "Brdf.frag")
                        .CreatePipeline();


            Engine::Get().GetGraphicsThread().SubmitCommands([&] (CommandBuffer& commandBuffer) -> bool {
                commandBuffer.BindPipeline(irradiancePipeline);

                ColorAttachmentInfo colorAttachment = m_lutTexture->GetColorAttachmentInfo(
                    ImageSubResource{
                            .type = ImageResourceType::e2D,
                            .baseMipmapLevel = 0,
                            .baseLayer = 0,
                            .mipmapLevelCount = 1,
                            .layerCount = 1
                        });


            DynamicRenderPassInfo renderPassInfo;
            renderPassInfo.colorAttachments = std::span(&colorAttachment, 1);
            renderPassInfo.extent = {m_lutTexture->GetExtent().x, m_lutTexture->GetExtent().y};

            commandBuffer.BeginRenderPass(renderPassInfo);

            commandBuffer.Draw(4, 1);

            commandBuffer.EndRenderPass();
            return true;
        });
        Engine::Get().GetGraphicsThread().WaitForFence();
    }

    Engine::Get().GetGraphicsThread().SubmitCommands([&] (const CommandBuffer& commandBuffer) -> bool {
        commandBuffer.TransferImageLayoutAndQueue(*m_envMap, vk::ImageLayout::eShaderReadOnlyOptimal, 0);
        commandBuffer.TransferImageLayoutAndQueue(*m_irradianceMap, vk::ImageLayout::eShaderReadOnlyOptimal, 0);
        commandBuffer.TransferImageLayoutAndQueue(*m_prefilterMap, vk::ImageLayout::eShaderReadOnlyOptimal, 0);
        commandBuffer.TransferImageLayoutAndQueue(*m_lutTexture, vk::ImageLayout::eShaderReadOnlyOptimal, 0);
        return true;
    });
    Engine::Get().GetGraphicsThread().WaitForFence();


    GetRenderer().GetEnvMapPipelineLayout()->GetDescriptor({0, 1})
        ->SetResource({ImageResource{
            .subresource = ImageSubResource{
                .type = ImageResourceType::eCube,
                .baseMipmapLevel = 0,
                .baseLayer = 0,
                .mipmapLevelCount = 1,
                .layerCount = 6,
                },
            .image = m_envMap.get(),
            .sampler = m_iblSampler.get()
        }}, 0);

    GetRenderer().GetLightingPassPipelineLayout()->GetDescriptor({0, 2})
    ->SetResource({ImageResource{
        .subresource = ImageSubResource{
            .type = ImageResourceType::eCube,
            .baseMipmapLevel = 0,
            .baseLayer = 0,
            .mipmapLevelCount = 1,
            .layerCount = 6,
            },
        .image = m_irradianceMap.get(),
        .sampler = m_iblSampler.get()
    }}, 0);

    GetRenderer().GetLightingPassPipelineLayout()->GetDescriptor({0, 3})
    ->SetResource({ImageResource{
        .subresource = ImageSubResource{
            .type = ImageResourceType::eCube,
            .baseMipmapLevel = 0,
            .baseLayer = 0,
            .mipmapLevelCount = 5,
            .layerCount = 6,
            },
        .image = m_prefilterMap.get(),
        .sampler = m_iblSampler.get()
    }}, 0);

    GetRenderer().GetLightingPassPipelineLayout()->GetDescriptor({0, 4})
    ->SetResource({ImageResource{
        .subresource = ImageSubResource{
            .type = ImageResourceType::e2D,
            .baseMipmapLevel = 0,
            .baseLayer = 0,
            .mipmapLevelCount = 1,
            .layerCount = 1,
            },
        .image = m_lutTexture.get(),
        .sampler = m_iblSampler.get()
    }}, 0);
}

std::vector<DefualtVertex> ResourceManager::LoadVertices(const fastgltf::Primitive& primitive, const fastgltf::Asset& asset) {
    const uint32_t vertexCount = asset.accessors[primitive.attributes[0].accessorIndex].count;

    std::vector<DefualtVertex> vertices(vertexCount);

    {
        auto* attrib = primitive.findAttribute("POSITION");
        if (attrib == primitive.attributes.cend()) {
            LogError("Primitive has no {} attribute", attrib->name);
        }
        const auto accessorIndex = attrib->accessorIndex;
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, asset.accessors[accessorIndex], [&](fastgltf::math::fvec3 value, std::size_t idx) {
            vertices[idx].position.x = glm::detail::toFloat16(value.x());
            vertices[idx].position.y = glm::detail::toFloat16(value.y());
            vertices[idx].position.z = glm::detail::toFloat16(value.z());
            vertices[idx].position.w = 0;
            //vertices[idx].boneIndices = {0, 0, 0, 0};
            //vertices[idx].boneWeights = {0.0f, 0.0f, 0.0f, 0.0f};
        });
    }
    {
        auto* attrib = primitive.findAttribute("NORMAL");
        if (attrib == primitive.attributes.cend()) {
            LogError("Primitive has no {} attribute", attrib->name);
        }
        const auto accessorIndex = attrib->accessorIndex;
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, asset.accessors[accessorIndex], [&](fastgltf::math::fvec3 value, std::size_t idx) {
            vertices[idx].normal.x = glm::detail::toFloat16(value.x());
            vertices[idx].normal.y = glm::detail::toFloat16(value.y());
            vertices[idx].normal.z = glm::detail::toFloat16(value.z());
            vertices[idx].normal.w = 0;
        });
    }
    {
        auto* attrib = primitive.findAttribute("TEXCOORD_0");
        if (attrib == primitive.attributes.cend()) {
            LogError("Primitive has no {} attribute", attrib->name);
        }
        const auto accessorIndex = attrib->accessorIndex;
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(asset, asset.accessors[accessorIndex], [&](fastgltf::math::fvec2 value, std::size_t idx) {
            vertices[idx].uv.x = glm::detail::toFloat16(value.x());
            vertices[idx].uv.y = glm::detail::toFloat16(value.y());
        });
    }
    {
        auto* attrib = primitive.findAttribute("TANGENT");
        if (attrib == primitive.attributes.cend()) {
            LogError("Primitive has no {} attribute", attrib->name);
        }
        const auto accessorIndex = attrib->accessorIndex;
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(asset, asset.accessors[accessorIndex], [&](fastgltf::math::fvec4 value, std::size_t idx) {
            vertices[idx].tangent.x = glm::detail::toFloat16(value.x());
            vertices[idx].tangent.y = glm::detail::toFloat16(value.y());
            vertices[idx].tangent.z = glm::detail::toFloat16(value.z());
            vertices[idx].tangent.w = glm::detail::toFloat16(value.w());
        });
    }

/*    {
        auto* attrib = primitive.findAttribute("JOINTS_0");
        if (!attrib) {
            _ERROR("Primitive " + std::string(attrib->name) + " has no {}POSITION attribute");
            return nullptr;
        }
        auto accessorIndex = attrib->accessorIndex;
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(asset, asset.accessors[accessorIndex], [&](fastgltf::math::fvec2 value, std::size_t idx) {
            vertices[idx].uv = {value.x(), value.y()};
        });
    }
    {
        auto* attrib = primitive.findAttribute("WEIGHTS_0");
        if (!attrib) {
            _ERROR("Primitive " + std::string(attrib->name) + " has no {}POSITION attribute");
            return nullptr;
        }
        auto accessorIndex = attrib->accessorIndex;
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(asset, asset.accessors[accessorIndex], [&](fastgltf::math::fvec2 value, std::size_t idx) {
            vertices[idx].uv = {value.x(), value.y()};
        });
    }
*/
    return vertices;
}

std::vector<uint32_t> ResourceManager::LoadIndices(const fastgltf::Primitive& primitive, const fastgltf::Asset& asset) {
    const uint32_t indexCount = asset.accessors[primitive.indicesAccessor.value()].count;

    std::vector<uint32_t> indices(indexCount);

    fastgltf::iterateAccessorWithIndex<uint32_t>(asset, asset.accessors[primitive.indicesAccessor.value()], [&](const uint32_t value, const std::size_t idx) {
        indices[idx] = value;
    });

    return indices;
}

void ResourceManager::LoadMeshGltf(const fastgltf::Mesh& meshGltf, const fastgltf::Asset& asset) {
    // Mesh already exists
    if (const auto it = m_meshes.find(std::string(meshGltf.name)); it != m_meshes.end())
        return;

    std::vector<Submesh*> submeshes;
    std::shared_ptr<Mesh<DefualtVertex, DefaultInstanceData>> mesh;

    for (const auto& primative : meshGltf.primitives) {
        const auto vertices = LoadVertices(primative, asset);
        const auto indices = LoadIndices(primative, asset);

        auto vertices_staging_buffer = RawBuffer(vertices.size() * sizeof(vertices[0]), vk::BufferUsageFlagBits::eTransferSrc, MemoryType::eCpuReadWrite);
        auto indices_staging_buffer = RawBuffer(indices.size() * sizeof(indices[0]), vk::BufferUsageFlagBits::eTransferSrc, MemoryType::eCpuReadWrite);

        {
            auto* mapped_ptr = vertices_staging_buffer.MapMemory();
                memcpy(mapped_ptr, vertices.data(), vertices.size() * sizeof(vertices[0]));
            vertices_staging_buffer.UnmapMemory();
        }

        {
            auto* mapped_ptr = indices_staging_buffer.MapMemory();
            memcpy(mapped_ptr, indices.data(), indices.size() * sizeof(indices[0]));
            indices_staging_buffer.UnmapMemory();
        }

        uint32_t material_index = 0;
        if (primative.materialIndex.has_value())
            material_index = primative.materialIndex.value();

        Engine::Get().GetGraphicsThread().SubmitCommands([&] (CommandBuffer& commandBuffer) -> bool {
            auto* submesh = GetRenderer().GetMeshManager().CreateSubmesh(commandBuffer, material_index, vertices_staging_buffer, indices_staging_buffer);
            submeshes.emplace_back(submesh);
            return true;
        });

        Engine::Get().GetGraphicsThread().WaitForFence();
        Engine::Get().GetGraphicsThread().DestroyDeferredDestroyBuffer();
    }

    Engine::Get().GetGraphicsThread().SubmitCommands([&] (CommandBuffer& commandBuffer) -> bool {
        mesh = GetRenderer().GetMeshManager().CreateMesh(commandBuffer, submeshes);
        return true;
    });

    m_meshes.emplace(std::string(meshGltf.name), mesh);
}

void ResourceManager::LoadImageGltf(const fastgltf::Image& imageGltf, const fastgltf::Asset& asset, const ImageFormat format) {
    auto [img, isNotExists] = m_textures.try_emplace(std::string(imageGltf.name), Texture());
    std::shared_ptr<Image> image = nullptr;

    if (!isNotExists)
        return;

    auto createInfo = ImageCreateInfo {
            {1, 1, 1},
            format,
            ImageType::e2D,
            ImageUsageBit::eSampled | ImageUsageBit::eTransferSrc | ImageUsageBit::eTransferDst,
            MemoryType::eDeviceLocal,
        };

    std::visit(fastgltf::visitor {
        []([[maybe_unused]] auto& arg) {},
        [&](const fastgltf::sources::URI& filePath) {
            int width, height, nrChannels;

            const std::string path(filePath.uri.path().begin(), filePath.uri.path().end());
            uint8_t* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);

            createInfo.extent = {width, height, 1};
            createInfo.mipmapCount = static_cast<uint32_t>(floor(log2(std::max(width, height))) + 1);
            image = Image::New(createInfo);

            Engine::Get().GetGraphicsThread().SubmitCommands([&] (CommandBuffer& commandBuffer) -> bool {
                commandBuffer.UploadData(*image, ImageSubResource{}, data, width * height * 4, 0);
                commandBuffer.GenerateMipMaps(*image);
                commandBuffer.TransferImageLayoutAndQueue(*image, vk::ImageLayout::eShaderReadOnlyOptimal, 0);
                return true;
            });

            stbi_image_free(data);
            Engine::Get().GetGraphicsThread().WaitForFence();
        },
        [&](const fastgltf::sources::Array& vector) {
            int width, height, nrChannels;
            uint8_t *data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(vector.bytes.data()), static_cast<int>(vector.bytes.size()), &width, &height, &nrChannels, 4);
            
            createInfo.extent = {width, height, 1};
            createInfo.mipmapCount = static_cast<uint32_t>(floor(log2(std::max(width, height))) + 1);
            image = Image::New(createInfo);

            Engine::Get().GetGraphicsThread().SubmitCommands([&] (CommandBuffer& commandBuffer) -> bool {
                commandBuffer.UploadData(*image, ImageSubResource{}, data, width * height * 4, 0);
                commandBuffer.GenerateMipMaps(*image);
                commandBuffer.TransferImageLayoutAndQueue(*image, vk::ImageLayout::eShaderReadOnlyOptimal, 0);
                return true;
            });

            stbi_image_free(data);
            Engine::Get().GetGraphicsThread().WaitForFence();
        },
        [&](const fastgltf::sources::BufferView& view) {
            auto& bufferView = asset.bufferViews[view.bufferViewIndex];
            auto& buffer = asset.buffers[bufferView.bufferIndex];

            std::visit(fastgltf::visitor {
                []([[maybe_unused]] auto& arg) {},
                [&](const fastgltf::sources::Array& vector) {

                    int width, height, nrChannels;
					uint8_t* data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(vector.bytes.data() + bufferView.byteOffset),
					                                            static_cast<int>(bufferView.byteLength), &width, &height, &nrChannels, 4);
                    createInfo.extent = {width, height, 1};
                    createInfo.mipmapCount = static_cast<uint32_t>(floor(log2(std::max(width, height))) + 1);
                    image = Image::New(createInfo);

                    Engine::Get().GetGraphicsThread().SubmitCommands([&] (CommandBuffer& commandBuffer) -> bool {
                        commandBuffer.UploadData(*image, ImageSubResource{}, data, width * height * 4, 0);
                        commandBuffer.GenerateMipMaps(*image);
                        commandBuffer.TransferImageLayoutAndQueue(*image, vk::ImageLayout::eShaderReadOnlyOptimal, 0);
                        return true;
                    });

                    stbi_image_free(data);
                    Engine::Get().GetGraphicsThread().WaitForFence();
                    }
            }, buffer.data);
        },
    }, imageGltf.data);

    if (image == nullptr)
        LogError("image is null");

    img->second = GetRenderer().AddTexture(image, GetRenderer().GetSampler());
}

void ResourceManager::LoadMaterialGltf(const fastgltf::Material& materialGltf, const fastgltf::Asset& asset) {
    auto it = m_materials.try_emplace(std::string(materialGltf.name), Material());
    if (!it.second)
        return;

    auto& material = it.first->second;
    material.handle = m_materialBuffer->count;
    material.data = &m_materialBuffer->array[m_materialBuffer->count++];

    {
        if (const auto& albedoTex = materialGltf.pbrData.baseColorTexture; albedoTex.has_value()) {
            auto imageIndex = asset.textures[albedoTex->textureIndex].imageIndex;
            const auto image = asset.images[imageIndex.value()];
            const auto name = std::string(image.name);
            if (const auto texture = GetTexture(name); texture.image == nullptr)
                LoadImageGltf(image, asset, ImageFormat::eR8G8B8A8Srgb);
            material.data->albedoTex = GetTexture(name).handle;
        }
        else {
            material.data->albedoValue = glm::vec4(
                materialGltf.pbrData.baseColorFactor.x(), 
                materialGltf.pbrData.baseColorFactor.y(),
                materialGltf.pbrData.baseColorFactor.z(), 
                materialGltf.pbrData.baseColorFactor.w());
        }
    }

    {
        if (const auto& normalTex = materialGltf.normalTexture; normalTex.has_value()) {
            auto imageIndex = asset.textures[normalTex->textureIndex].imageIndex;
            const auto image = asset.images[imageIndex.value()];
            const auto name = std::string(image.name);
            if (const auto texture = GetTexture(name); texture.image == nullptr)
                LoadImageGltf(image, asset, ImageFormat::eR8G8B8A8Unorm);
            material.data->normalTex = GetTexture(name).handle;
        }
    }
    
    {
        if (const auto& metallicRoughnessTex = materialGltf.pbrData.metallicRoughnessTexture; metallicRoughnessTex.has_value()) {
            auto imageIndex = asset.textures[metallicRoughnessTex->textureIndex].imageIndex;
            const auto image = asset.images[imageIndex.value()];
            const auto name = std::string(image.name);
            if (const auto texture = GetTexture(name); texture.image == nullptr)
                LoadImageGltf(image, asset, ImageFormat::eR8G8B8A8Unorm);
            material.data->metallicRoughnessTex = GetTexture(name).handle;
        } else {
            material.data->roughnessValue = materialGltf.pbrData.roughnessFactor;
            material.data->metallicValue = materialGltf.pbrData.metallicFactor;
        }
    }
}

void ResourceManager::LoadNodeGltf(const fastgltf::Node& node, const fastgltf::Asset& asset, Scene* scene) {
    auto& entity = scene->CreateEntity(node.name);

/*    if (node.cameraIndex.has_value()) {
    }
*/
    if (node.meshIndex.has_value()) {
        const std::string mesh_name(asset.meshes[node.meshIndex.value()].name);

        const auto mesh = m_meshes.at(mesh_name);
        //mesh->SetInstanceData(DefaultInstanceData);
        //entity.AddComponent<RenderObject>(new RenderObject(&entity, GetMesh(meshName)));

        if (auto* trs = std::get_if<fastgltf::TRS>(&node.transform)) {
            fastgltf::math::fmat4x4 model_mtx;
            model_mtx = translate(model_mtx, trs->translation);
            model_mtx = rotate(model_mtx, trs->rotation);
            model_mtx = scale(model_mtx, trs->scale);

            const auto glm_model_mtx = glm::mat4(
                *reinterpret_cast<glm::vec4*>(model_mtx.col(0).data()),
                *reinterpret_cast<glm::vec4*>(model_mtx.col(1).data()),
                *reinterpret_cast<glm::vec4*>(model_mtx.col(2).data()),
                *reinterpret_cast<glm::vec4*>(model_mtx.col(3).data()));

            mesh->SetInstanceData(DefaultInstanceData{glm_model_mtx});
        }
/*
*         else if (auto* transform = std::get_if<fastgltf::math::fmat4x4>(&node.transform)) {
        }
 */
    }

/*    if (node.lightIndex.has_value()) {
    }*/
}

void ResourceManager::LoadAssetGltf(const std::string& name) {
    fastgltf::Parser parser { fastgltf::Extensions::KHR_lights_punctual };
    const std::filesystem::path path("./Assets/" + name);

    auto data = fastgltf::GltfDataBuffer::FromPath(path);
    if (data.error() != fastgltf::Error::None) {
        LogError("Failed to load glTF asset: {}, error: {}", name, fastgltf::getErrorMessage(data.error()));
        return;
    }
    
    auto asset = parser.loadGltfBinary(data.get(), path.parent_path());
    if (asset.error() != fastgltf::Error::None) {
        LogError("Failed to parse glTF asset: {}, error: {}", name, fastgltf::getErrorMessage(data.error()));
        return;
    }

    for (auto& mesh : asset->meshes)
        LoadMeshGltf(mesh, asset.get());

    for (auto& material : asset->materials)
        LoadMaterialGltf(material, asset.get());

    //asset->lights;

    for (auto& node : asset->nodes)
        LoadNodeGltf(node, asset.get(), Engine::Get().GetCurrentScene());
}

void ResourceManager::LoadResource(const Scene& scene) {
    //load image
    {
        const auto& usedImages = scene.GetUsedImages();
        for (const auto& imageName : usedImages) {
            LoadImage(imageName, ImageFormat::eR8G8B8A8Unorm);
        }
    }
    {
        const auto& usedEnvMap = scene.GetUsedEnvMap();
        LoadEnvMap(usedEnvMap);
    }

    //I don't have a metarial system yet
/*    {
        const auto& usedMaterials = scene.GetUsedMaterials();
        for (const auto& materialName : usedMaterials) {
            m_materials.try_emplace(materialName);
        }
    }*/
    //load asset
    {
        const auto& usedAssets = scene.GetUsedAssets();
        for (const auto& assetName : usedAssets) {
            LoadAssetGltf(assetName);
        }
    }
}