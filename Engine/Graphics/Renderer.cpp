#include "Graphics/Renderer.hpp"

#include "Graphics/Pipeline.hpp"
#include "Graphics/Sampler.hpp"
#include "Window/Window.hpp"
#include "Engine.hpp"

#include <glm/detail/type_quat.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl3.h>
#include <imgui/imgui_impl_vulkan.h>

#include "Utility/Counter.hpp"

Renderer::~Renderer() {
    VkContext::Get().GetDevice().waitIdle();

    shadowPassPipeline.reset();
    shadowBlurPipeline.reset();
    geometryPassPipeline.reset();
    lightingPassPipeline.reset();
    envMapPipeline.reset();

    if (ImGui::GetCurrentContext()) {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplSDL3_Shutdown();
    }

    for (auto& tex: m_textures)
        tex.image.reset();

    delete directionalLight;
    delete m_sampler;
    delete m_depthSampler;
}

void Renderer::CreateResources() {
    {
        m_sampler = new Sampler(vk::Filter::eLinear, vk::SamplerAddressMode::eRepeat, vk::CompareOp::eNever);
        m_depthSampler = new Sampler(vk::Filter::eLinear, vk::SamplerAddressMode::eRepeat, vk::CompareOp::eLess);
    }

    {
        m_swapchain = Swapchain::New(vk::ColorSpaceKHR::eSrgbNonlinear, vk::Format::eR8G8B8A8Unorm, vk::PresentModeKHR::eImmediate);
    }

    {
        depthImage = Image::New(ImageCreateInfo{
            {GetWindow().GetExtent(), 1},
            ImageFormat::eD32Sfloat,
            ImageType::e2D,
            ImageUsageBit::eDepthAttachment,
            MemoryType::eDeviceLocal,
            1,
            1,
            vk::ImageLayout::eDepthAttachmentOptimal
        });
    }

    {
        shadowDepthImage = Image::New(ImageCreateInfo{
            {ShadowMapSize, ShadowMapSize, 1},
            ImageFormat::eD32Sfloat,
            ImageType::e2D,
            ImageUsageBit::eDepthAttachment,
            MemoryType::eDeviceLocal,
            1,
            1,
            vk::ImageLayout::eDepthAttachmentOptimal
        });
    }

    {
        vk::VertexInputBindingDescription bindingDesc;
        vk::VertexInputAttributeDescription attribDesc;

        bindingDesc.setBinding(0)
                    .setStride(28);//f16vec4 * 3 + f16vec2

        attribDesc.setBinding(0)
                    .setFormat(vk::Format::eR16G16B16A16Sfloat)
                    .setLocation(0)
                    .setOffset(0);

        std::vector<vk::Format> formats;
        formats.emplace_back(vk::Format::eR32G32B32A32Sfloat);

        shadowPassPipeline = GraphicsPipeline::New();
        shadowPassPipeline->SetMultisampleStateInfo()
                        .SetDynamicStateInfo()
                        .SetInputAssamblyStateInfo(vk::PrimitiveTopology::eTriangleList)
                        .SetResterStateInfo(vk::CullModeFlagBits::eNone)
                        .SetVertexInputState(bindingDesc, {attribDesc})
                        .SetDepthStencilState()
                        .SetViewportState(vk::Viewport(0, 0, ShadowMapSize, ShadowMapSize, 0, 1.f),
                                        vk::Rect2D({0,0}, vk::Extent2D(ShadowMapSize, ShadowMapSize)))
                        .SetRenderingCreateInfo(formats, vk::Format::eD32Sfloat)
                        .SetShaderStages("ShadowPass.vert", "ShadowPass.frag")
                        .CreatePipeline();
    }

    {
        shadowBlurPipeline = ComputePipeline::New("ShadowBlur.comp");
    }

    {
        vk::VertexInputBindingDescription bindingDesc;
        std::vector<vk::VertexInputAttributeDescription> attribDescs;

        bindingDesc.setBinding(0)
                    .setStride(28);//f16vec4 * 3 + f16vec2

        attribDescs.emplace_back(0, 0, vk::Format::eR16G16B16A16Sfloat, 0);
        attribDescs.emplace_back(1, 0, vk::Format::eR16G16B16A16Sfloat, 8);
        attribDescs.emplace_back(2, 0, vk::Format::eR16G16B16A16Sfloat, 16);
        attribDescs.emplace_back(3, 0, vk::Format::eR16G16Sfloat, 24);

        std::vector<vk::Format> formats;
        formats.emplace_back(vk::Format::eR16G16B16A16Sfloat);//albedo + roughness
        formats.emplace_back(vk::Format::eR16G16B16A16Sfloat);//normal + metallic
        formats.emplace_back(vk::Format::eR16G16B16A16Sfloat);//position + nothing

        geometryPassPipeline = GraphicsPipeline::New();
        geometryPassPipeline->SetMultisampleStateInfo()
                .SetDynamicStateInfo()
                .SetInputAssamblyStateInfo(vk::PrimitiveTopology::eTriangleList)
                .SetResterStateInfo(vk::CullModeFlagBits::eFront)
                .SetVertexInputState(bindingDesc, {attribDescs})
                .SetDepthStencilState()
                .SetViewportState(vk::Viewport(0, 0,
                                static_cast<float>(m_swapchain->GetExtent().width),
                                static_cast<float>(m_swapchain->GetExtent().height), 0, 1.f),
                                vk::Rect2D({0,0}, m_swapchain->GetExtent()))
                .SetRenderingCreateInfo(formats, vk::Format::eD32Sfloat)
                .SetShaderStages("GeometryPass.vert", "GeometryPass.frag")
                .CreatePipeline();
    }

    {
        std::vector<vk::Format> formats;
        formats.emplace_back(m_swapchain->GetFormat());

        lightingPassPipeline = GraphicsPipeline::New();
        lightingPassPipeline->SetMultisampleStateInfo()
                .SetDynamicStateInfo()
                .SetInputAssamblyStateInfo(vk::PrimitiveTopology::eTriangleStrip)
                .SetResterStateInfo(vk::CullModeFlagBits::eNone)
                .SetVertexInputState({}, {})
                .SetDepthStencilState(true, false, vk::CompareOp::eNotEqual)
                .SetViewportState(vk::Viewport(0, 0,
                    static_cast<float>(m_swapchain->GetExtent().width),
                    static_cast<float>(m_swapchain->GetExtent().height), 0, 1.f),
                                vk::Rect2D({0,0}, m_swapchain->GetExtent()))
                .SetRenderingCreateInfo(formats, vk::Format::eD32Sfloat)
                .SetShaderStages("LightingPass.vert", "LightingPass.frag")
                .CreatePipeline();
    }

    {
        std::vector<vk::Format> formats;
        formats.emplace_back(m_swapchain->GetFormat());

        envMapPipeline = GraphicsPipeline::New();
        envMapPipeline->SetMultisampleStateInfo()
                .SetDynamicStateInfo()
                .SetInputAssamblyStateInfo(vk::PrimitiveTopology::eTriangleList)
                .SetResterStateInfo(vk::CullModeFlagBits::eNone)
                .SetVertexInputState({}, {})
                .SetDepthStencilState(true, false, vk::CompareOp::eLessOrEqual)
                .SetViewportState(vk::Viewport(0, 0,
                    static_cast<float>(m_swapchain->GetExtent().width),
                    static_cast<float>(m_swapchain->GetExtent().height), 0, 1.f),
                                vk::Rect2D({0,0}, m_swapchain->GetExtent()))
                .SetRenderingCreateInfo(formats, vk::Format::eD32Sfloat)
                .SetShaderStages("EnvMap.vert", "EnvMap.frag")
                .CreatePipeline();
    }

    {
        ImageCreateInfo createInfo;
        createInfo.arraySize = 1;
        createInfo.extent = {m_swapchain->GetExtent().width, m_swapchain->GetExtent().height, 1};
        createInfo.format = vk::Format::eR16G16B16A16Sfloat;
        createInfo.memoryUsage = MemoryType::eDeviceLocal;
        createInfo.usage = ImageUsageBit::eColorAttachment | ImageUsageBit::eStorage;
        createInfo.mipmapCount = 1;
        createInfo.type = ImageType::e2D;
        createInfo.arraySize = 3;
        createInfo.initLayout = vk::ImageLayout::eGeneral;

        gBuffer = Image::New(createInfo);

        GetLightingPassPipelineLayout()->GetDescriptor({0, 0})->SetResource({
            ImageResource{
                .subresource = {
                    .type = ImageResourceType::e2DArray,
                    .layerCount = 3
                },
                .image = gBuffer.get(),
                }
        }, 0);
    }

    {//TODO: switch to dynamic rendering
        geometryPass.AddColorAttachment(gBuffer->GetColorAttachmentInfo(ImageSubResource {
                .type = ImageResourceType::e2D,
                .baseLayer = 0,
                .layerCount = 1,
            }));

        geometryPass.AddColorAttachment(gBuffer->GetColorAttachmentInfo(ImageSubResource {
             .type = ImageResourceType::e2D,
             .baseLayer = 1,
             .layerCount = 1,
            }));

        geometryPass.AddColorAttachment(gBuffer->GetColorAttachmentInfo(ImageSubResource {
             .type = ImageResourceType::e2D,
             .baseLayer = 2,
             .layerCount = 1,
            }));

        geometryPass.SetDepthAttachment(depthImage->GetDepthAttachmentInfo(ImageSubResource {
             .type = ImageResourceType::e2D,
             .baseLayer = 0,
             .layerCount = 1,
            }));
    }

/*
    *
        {
            lightingPass.AddColorAttachment(glm::vec4(0), VK_NULL_HANDLE);
            lightingPass.SetDepthStencilAttachment(
                vk::ClearDepthStencilValue(1.f, 0),
                depthImageView->GetHandle(),
                vk::AttachmentLoadOp::eLoad,
                vk::AttachmentStoreOp::eNone);
        }
 */

    {
        uint32_t texel = std::numeric_limits<uint32_t>::max();//max uint32_t value

        placeholderImage = Image::New(ImageCreateInfo{
            {1, 1, 1},
            ImageFormat::eR8G8B8A8Srgb,
            ImageType::e2D,
            ImageUsageBit::eSampled | ImageUsageBit::eTransferDst,
            MemoryType::eDeviceLocal,
            1,
            1,
        });

        Engine::Get().GetGraphicsThread().SubmitCommands([&] (CommandBuffer& commandBuffer) -> bool {
            commandBuffer.UploadData(*placeholderImage, {}, &texel, sizeof(uint32_t), 0);

            return true;
        });
    }

    {
        std::vector<ImageResource> placeholderArray(128);
        std::ranges::fill(placeholderArray, ImageResource{.subresource = {}, .image = placeholderImage.get(), .sampler = GetSampler()});
        GetGeometryPassPipelineLayout()->GetDescriptor({0, 1})->SetResource(placeholderArray, 0);

        m_textures.emplace_back(Texture{placeholderImage, GetSampler(), static_cast<uint32_t>(m_textures.size())});
    }

    {
        cameraBuffer = RawBuffer::New(sizeof(CameraGPU), vk::BufferUsageFlagBits::eUniformBuffer, MemoryType::eCpuWrite);
        cameraPtr = cameraBuffer->MapMemory<CameraGPU>();
        GetGeometryPassPipelineLayout()->GetDescriptor({0, 0})->SetResource({cameraBuffer.get()}, 0);
        GetEnvMapPipelineLayout()->GetDescriptor({0, 0})->SetResource({cameraBuffer.get()}, 0);
    }

    {
        cam_pos_buffer = RawBuffer::New(sizeof(glm::vec4), vk::BufferUsageFlagBits::eUniformBuffer, MemoryType::eCpuWrite);
        cam_pos = cam_pos_buffer->MapMemory<glm::vec4>();

        GetLightingPassPipelineLayout()->GetDescriptor({0, 7})->SetResource({cam_pos_buffer.get()}, 0);
    }

    {
        m_lightResourceBuffer = RawBuffer::New(sizeof(LightBuffer), vk::BufferUsageFlagBits::eUniformBuffer, MemoryType::eCpuWrite);
        m_lightBuffer = m_lightResourceBuffer->MapMemory<LightBuffer>();
        GetLightingPassPipelineLayout()->GetDescriptor({0, 1})->SetResource({m_lightResourceBuffer.get()}, 0);
    }

    InitImGui();

    {
        vk::QueryPoolCreateInfo createInfo;
        createInfo.setQueryCount(1)
                    .setQueryType(vk::QueryType::ePipelineStatistics)
                    .setPipelineStatistics(
                        vk::QueryPipelineStatisticFlagBits::eInputAssemblyPrimitives
                        | vk::QueryPipelineStatisticFlagBits::eVertexShaderInvocations
                    );

        shadowPassQueryPool = VkContext::Get().GetDevice().createQueryPool(createInfo);
    }

    {
        vk::QueryPoolCreateInfo createInfo;
        createInfo.setQueryCount(1)
                    .setQueryType(vk::QueryType::ePipelineStatistics)
                    .setPipelineStatistics(
                        vk::QueryPipelineStatisticFlagBits::eInputAssemblyPrimitives
                        | vk::QueryPipelineStatisticFlagBits::eFragmentShaderInvocations
                        | vk::QueryPipelineStatisticFlagBits::eVertexShaderInvocations
                        | vk::QueryPipelineStatisticFlagBits::eClippingPrimitives
                    );

        geometryPassQueryPool = VkContext::Get().GetDevice().createQueryPool(createInfo);
    }

    {
        vk::QueryPoolCreateInfo createInfo;
        createInfo.setQueryCount(1)
                    .setQueryType(vk::QueryType::ePipelineStatistics)
                    .setPipelineStatistics(
        vk::QueryPipelineStatisticFlagBits::eFragmentShaderInvocations
                    );

        lightingPassQueryPool = VkContext::Get().GetDevice().createQueryPool(createInfo);
    }

    {
        vk::QueryPoolCreateInfo createInfo;
        createInfo.setQueryCount(4)
                    .setQueryType(vk::QueryType::eTimestamp)
                    ;

        timeStrapQueryPool = VkContext::Get().GetDevice().createQueryPool(createInfo);
    }

    {
        auto properties = VkContext::Get().GetPhysicalDevice().getProperties();
        timestampPeriod = properties.limits.timestampPeriod;
    }

/*    {
        directionalLight = new DirectionalLight(
            glm::vec2(5),
            DirectionalLightData {
                .lightSpaceMatrix = {},
                .position = glm::vec3(300, 300, 300),
                .direction = -glm::vec3(0.078, 0.15, 0.2),
                .color = {1,1,1},
                .intensity = 3,
                .shadowMapIndex = 0,
        });
    }*/

    // {
    //     shadowBlurPipeline = ComputePipeline::New("ShadowBlur.comp");
    //     shadowBlurPipeline->GetPipelineLayout()->GetDescriptor({0, 0})->SetResource(
    //         {
    //             ImageResource{
    //                 .subresource = ImageSubResource {
    //                     .baseMipmapLevel = 0,
    //                     .mipmapLevelCount = 1
    //                 },
    //                 .image = shadow_maps[0]->image.get()
    //             },
    //             ImageResource{
    //                 .subresource = ImageSubResource {
    //                     .baseMipmapLevel = 1,
    //                     .mipmapLevelCount = 1
    //                 },
    //                 .image = shadow_maps[0]->image.get()
    //             },
    //     }, 0);
    // }
}

void Renderer::Render() {
    static bool first = true;

    cameraPtr->viewMtx = currentCamera->CalcViewMatrix();
    cameraPtr->projectionMtx = currentCamera->GetProjectionMtx();
    *cam_pos = glm::vec4(currentCamera->position, 1);

    if (first)
        RecordRenderCommandBuffers();

    Engine::Get().GetGraphicsThread().GetQueue().waitIdle();
    m_swapchain->AcquireNextImage();

    if constexpr (debug) {
        if (!first) { // get query results
            { // geometry pool
                const auto result =
                    geometryPassQueryPool.getResult<std::array<uint64_t, 4>>
                    (0, 1, 8, vk::QueryResultFlagBits::e64).second;

                drawedPrimativeCount = result[0];
                vertexShaderInvocations = result[1];
                clippingInvocations = result[2];
                fragmentShaderInvocations = result[3];
            }

            { // lighting pool
                const auto result =
                    lightingPassQueryPool.getResult<uint64_t>
                    (0, 1, 8, vk::QueryResultFlagBits::e64).second;

                lightingFragmentShaderInvocations = result;
            }

            { // timestrap
                const auto result =
                                timeStrapQueryPool.getResult<std::array<uint64_t, 4>>
                                (0, 4, sizeof(uint64_t), vk::QueryResultFlagBits::e64).second;

                geometryPassDuration = static_cast<double>(result[1] - result[0]) * static_cast<double>(timestampPeriod / 1'000'000.f);
                lightingPassDuration = static_cast<double>(result[3] - result[2]) * static_cast<double>(timestampPeriod / 1'000'000.f);
            }
        }

        geometryPassQueryPool.reset(0, 1);
        lightingPassQueryPool.reset(0, 1);
        timeStrapQueryPool.reset(0, 4);
    }

    Engine::Get().GetGraphicsThread().SubmitCommands([&] (CommandBuffer& command_buffer) -> bool {
        command_buffer.ExecuteCommandBuffer(render_command_buffers[m_swapchain->GetImageIndex()]);

        ColorAttachmentInfo colorAttachment = m_swapchain->GetColorAttachmentInfo({0,0,0,0}, AttachmentLoadOp::eLoad, AttachmentStoreOp::eStore);

        const DynamicRenderPassInfo renderPassInfo {
                .colorAttachments = std::span(&colorAttachment, 1),
                .depthAttachment = std::nullopt,
                .extent = {m_swapchain->GetExtent().width, m_swapchain->GetExtent().height},
                .offset = {0,0}
            };

        command_buffer.BeginRenderPass(renderPassInfo);

        { // ui drawing
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();

            ImGui::Begin("Profiler");
            ImGui::Text("Fps: %f", ImGui::GetIO().Framerate);
            ImGui::Text("Geometry Pass Duration: %.2fms", geometryPassDuration);
            ImGui::Text("Lighting Pass Duration: %.2fms", lightingPassDuration);

            if (ImGui::CollapsingHeader("GeometryPass")) {
                ImGui::Text("Drawed Primative Count: %lluk", drawedPrimativeCount/1000);
                ImGui::Text("Vertex Shader Invocations: %lluk", vertexShaderInvocations/1000);
                ImGui::Text("Fragment Shader Invocations: %lluk", fragmentShaderInvocations/1000);
                ImGui::Text("Clipping Invocations: %lluk", clippingInvocations/1000);
            }

            if (ImGui::CollapsingHeader("LightingPass")) {
                ImGui::Text("Fragment Shader Invocations: %lluk", lightingFragmentShaderInvocations/1000);
            }

            ImGui::End();

            ImGui::Render();

            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer.GetCommandBuffer());
        }

        command_buffer.EndRenderPass();

        return true;
    }, false);

    Engine::Get().GetGraphicsThread().PresentFrame(*m_swapchain);

    currentFrame = (currentFrame + 1) % MaxFramesInFlight;
    first = false;
}

LightHandle Renderer::CreateLight(const DirectionalLightData& data) const {
    if (++m_lightBuffer->directionalLightCount > 31)
        return LightUndefined;

    m_lightBuffer->directionalLightArray[m_lightBuffer->directionalLightCount - 1] = data;

    return m_lightBuffer->directionalLightCount - 1;
}

LightHandle Renderer::CreateLight(const SpotLightData& data) const {
    if (++m_lightBuffer->spotLightCount > 31)
        return LightUndefined;

    m_lightBuffer->spotLightArray[m_lightBuffer->spotLightCount - 1] = data;

    return m_lightBuffer->spotLightCount - 1;
}

LightHandle Renderer::CreateLight(const PointLightData& data) const {
    if (++m_lightBuffer->pointLightCount > 31)
        return LightUndefined;

    m_lightBuffer->pointLightArray[m_lightBuffer->pointLightCount - 1] = data;

    return m_lightBuffer->pointLightCount - 1;
}

void Renderer::InitImGui() const {
    LogInfo("Imgui initialize");
    
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    }

    {
        const auto& vkcontext = VkContext::Get();

        ImGui::StyleColorsDark();
        ImGui_ImplSDL3_InitForVulkan(GetWindow().GetSDLWindow());
        
        ImGui_ImplVulkan_InitInfo imguiVulkanInitInfo = {};
        imguiVulkanInitInfo.Instance = *vkcontext.GetInstance();
        imguiVulkanInitInfo.Device = *vkcontext.GetDevice();
        imguiVulkanInitInfo.PhysicalDevice = *vkcontext.GetPhysicalDevice();
        imguiVulkanInitInfo.UseDynamicRendering = true;
        imguiVulkanInitInfo.Queue = *vkcontext.GetGraphicsQueue();
        imguiVulkanInitInfo.QueueFamily = vkcontext.GetGraphicsQueueFamily();
        imguiVulkanInitInfo.DescriptorPoolSize = 256;
        imguiVulkanInitInfo.MinImageCount = m_swapchain->GetImages().size();
        imguiVulkanInitInfo.ImageCount = m_swapchain->GetImages().size();

        const auto format = static_cast<VkFormat>(m_swapchain->GetFormat());

        imguiVulkanInitInfo.PipelineInfoMain.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        imguiVulkanInitInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        imguiVulkanInitInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &format;
        imguiVulkanInitInfo.PipelineInfoMain.PipelineRenderingCreateInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;
        imguiVulkanInitInfo.PipelineInfoMain.PipelineRenderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

        ImGui_ImplVulkan_Init(&imguiVulkanInitInfo);
    }
}

void Renderer::RecordRenderCommandBuffers() {
    for (const auto i : Counter(3)) {

        vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
        commandBufferAllocateInfo.setCommandPool(Engine::Get().GetGraphicsThread().GetCommandPool());
        commandBufferAllocateInfo.setCommandBufferCount(1);
        commandBufferAllocateInfo.setLevel(vk::CommandBufferLevel::eSecondary);

        auto& command_buffer =
            render_command_buffers.emplace_back(VkContext::Get().GetDevice().allocateCommandBuffers(commandBufferAllocateInfo)[0],
                Engine::Get().GetGraphicsThread().GetQueueFamilyIndex());

        vk::CommandBufferInheritanceInfo inheritance_info{};
        inheritance_info.pipelineStatistics
        = vk::QueryPipelineStatisticFlagBits::eInputAssemblyPrimitives
        | vk::QueryPipelineStatisticFlagBits::eFragmentShaderInvocations
        | vk::QueryPipelineStatisticFlagBits::eVertexShaderInvocations
        | vk::QueryPipelineStatisticFlagBits::eClippingPrimitives;

        vk::CommandBufferBeginInfo begin_info{};
        begin_info.pInheritanceInfo = &inheritance_info;

        command_buffer.GetCommandBuffer().begin(begin_info);

        PipelineBarrierInfo memoryBarrier;

        {// Geometry Pass
            command_buffer.BeginRenderPass(geometryPass, {m_swapchain->GetExtent().width, m_swapchain->GetExtent().height}, {0,0});
            command_buffer.BindPipeline(*geometryPassPipeline);

            command_buffer.GetCommandBuffer().beginQuery(geometryPassQueryPool, 0, {});
            command_buffer.GetCommandBuffer().writeTimestamp2(vk::PipelineStageFlagBits2::eTopOfPipe, timeStrapQueryPool, 0);

            m_mesh_manager.Draw(command_buffer, *geometryPassPipeline->GetPipelineLayout());

            command_buffer.GetCommandBuffer().writeTimestamp2(vk::PipelineStageFlagBits2::eBottomOfPipe, timeStrapQueryPool, 1);
            command_buffer.GetCommandBuffer().endQuery(geometryPassQueryPool, 0);

            command_buffer.EndRenderPass();
        }// Geometry Pass

        memoryBarrier = {
            .srcStageFlags = PipelineStageBit::eColorAttachmentOutput,
            .dstStageFlags = PipelineStageBit::eFragmentShader,
            .srcAccessFlags = AccessBit::eColorAttachmentWrite,
            .dstAccessFlags = AccessBit::eShaderStorageRead
        };

        command_buffer.PipelineBarrier(std::span{&memoryBarrier, 1});

        {// lighting pass and ui

            ColorAttachmentInfo colorAttachment = m_swapchain->GetColorAttachmentInfo(i, {0,0,0,0}, AttachmentLoadOp::eClear, AttachmentStoreOp::eStore);
            DepthAttachmentInfo depthAttachment = depthImage->GetDepthAttachmentInfo(ImageSubResource{}, {1.f, 0}, AttachmentLoadOp::eLoad, AttachmentStoreOp::eNone);

            const DynamicRenderPassInfo renderPassInfo {
                .colorAttachments = std::span(&colorAttachment, 1),
                .depthAttachment = depthAttachment,
                .extent = {m_swapchain->GetExtent().width, m_swapchain->GetExtent().height},
                .offset = {0,0}
            };

            command_buffer.BeginRenderPass(renderPassInfo);
            command_buffer.BindPipeline(*lightingPassPipeline);

            command_buffer.GetCommandBuffer().beginQuery(lightingPassQueryPool, 0, {});
            command_buffer.GetCommandBuffer().writeTimestamp2(vk::PipelineStageFlagBits2::eTopOfPipe, timeStrapQueryPool, 2);

            command_buffer.Draw(4, 1);

            command_buffer.GetCommandBuffer().writeTimestamp2(vk::PipelineStageFlagBits2::eBottomOfPipe, timeStrapQueryPool, 3);
            command_buffer.GetCommandBuffer().endQuery(lightingPassQueryPool, 0);

            command_buffer.BindPipeline(*envMapPipeline);
            command_buffer.Draw(36, 1);

            command_buffer.EndRenderPass();
        }// lighting pass and ui

        command_buffer.OptimalSwapchainImageLayout(*m_swapchain, i);
        command_buffer.GetCommandBuffer().end();
    }
}
