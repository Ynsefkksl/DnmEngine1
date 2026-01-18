#include "Engine.hpp"

#include "Graphics/Renderer.hpp"
#include "Graphics/ShaderCache.hpp"
#include "Utility/EngineDefines.hpp"
#include "Utility/LoadShared.hpp"
#include "Utility/Log.hpp"
#include "Entity/Scene.hpp"
#include "Window/Window.hpp"

#include <cstdint>

#include <nlohmann/json.hpp>

#include <string>

void Engine::Init() {
    m_window = new Window("Dnm Engine", {});

    auto& log = Log::Get();
    log.Init(std::filesystem::current_path());

    m_threadPool = new ThreadPool;
    m_shaderCache = new ShaderCache;

    auto& vulkanContext = VkContext::Get();
    vulkanContext.Init();

    m_graphicsThread = new GraphicsThread(
        vulkanContext.GetGraphicsQueue(), vulkanContext.GetGraphicsQueueFamily());

    if (vulkanContext.IsComputeQueueExists())
        m_computeThread = new GraphicsThread(
            vulkanContext.GetComputeQueue(), vulkanContext.GetComputeQueueFamily());

    if (vulkanContext.IsTransferQueueExists())
        m_transferThread = new GraphicsThread(
            vulkanContext.GetTransferQueue(), vulkanContext.GetTransferQueueFamily());

    m_renderer = new Renderer;
    // Prevent resource access to the renderer before it is initialized
    m_renderer->CreateResources();

    m_resourceManager = new ResourceManager;

    LoadScriptFuncs();

    CreateScene("dnm");
}

void Engine::Run() {
    LogInfo("Engine run");

    for (const auto &scene : m_scenes | std::views::values)
        scene->RunScripts_initialize();

    while (m_window->PollEvents()) {
        if (m_window->IsKeyHeld(Key::eEscape))
            break;

        for (const auto &scene: m_scenes | std::views::values)
            scene->RunScripts_update();

        m_renderer->Render();
    }
}

void Engine::CleanUp() {
    for (const auto &scene: m_scenes | std::views::values)
        scene->RunScripts_cleanUp();

    m_scenes.clear();

    delete m_renderer;
    delete m_resourceManager;
    delete m_threadPool;
    delete m_shaderCache;
    delete m_window;

    delete m_graphicsThread;
    delete m_computeThread;
    delete m_transferThread;

    auto& vulkanContext = VkContext::Get();
    vulkanContext.CleanUp();

    auto& log = Log::Get();
    log.CleanUp();
}

Scene* Engine::CreateScene(const std::string_view name) {
    auto [iter, inserted] = m_scenes.try_emplace(
        std::string(name),
        new Scene(name)
    );

    if (currentScene == nullptr)
        currentScene = iter->second;

    m_resourceManager->LoadResource(*iter->second);

    return iter->second;
}

Scene* Engine::GetScene(const std::string& name) {
    const auto it = m_scenes.find(name);
    return (it != m_scenes.end()) ? it->second : nullptr;
}

Engine::ScriptLoader Engine::GetScriptLoader(const std::string& name) {
    const auto it = m_scriptsLoader.find(name);
    return it != m_scriptsLoader.end() ? it->second : nullptr;
}

void Engine::LoadScriptFuncs() {
    const auto scriptsPath = std::filesystem::path("Scripts");

    if (!(std::filesystem::exists(scriptsPath) && std::filesystem::is_directory(scriptsPath)))
        LogError("Don't found scripts directory");

    for (const auto& entry : std::filesystem::directory_iterator(scriptsPath)) {
        if (entry.path().filename().extension().string() != ".cpp")
            continue;

        std::string nameShared = entry.path().filename().string();
        nameShared = nameShared.substr(0, nameShared.size() - entry.path().filename().extension().string().size());

        ScriptLoader script;

        script = LoadShared<ScriptLoader>(nameShared, "GetScript");

        m_scriptsLoader.try_emplace(nameShared, script);
    }
}

extern "C" __declspec(dllexport) void InitEngine() {
    auto& engine = Engine::Get();
    engine.Init();
    engine.Run();
    engine.CleanUp();
}

extern "C" __declspec(dllexport) void CloseEngine() {
    auto& engine = Engine::Get();
    engine.CleanUp();
}