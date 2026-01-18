#pragma once

#include <mutex>

#include "Window/Window.hpp"
#include <string>
#include <unordered_map>
#include <string_view>

#define SCRIPT_LOADER(className) extern "C" __declspec(dllexport) className* GetScript(Entity* entity) { return new className(entity); }

extern "C" __declspec(dllexport) void InitEngine();
extern "C" __declspec(dllexport) void CloseEngine();

class Renderer;
class Entity;
class Script;
class Scene;
class GraphicsThread;
class ResourceManager;
class ThreadPool;
class ShaderCache;

constexpr uint32_t ShadowMapSize = 1024;

class ENGINE_API Engine {
    Engine() = default;

    using ScriptLoader = Script*(*)(Entity* entity);
public:
    static Engine& Get() {
        static Engine instance;
        return instance;
    }

    [[nodiscard]] FORCE_INLINE ResourceManager& GetResourceManager() const noexcept { return *m_resourceManager; }
    [[nodiscard]] FORCE_INLINE ThreadPool& GetThreadPool() const noexcept { return *m_threadPool; }
    [[nodiscard]] FORCE_INLINE ShaderCache& GetShaderCache() const noexcept { return *m_shaderCache; }
    [[nodiscard]] FORCE_INLINE Renderer& GetRenderer() const noexcept { return *m_renderer; }
    [[nodiscard]] FORCE_INLINE Window& GetWindow() const noexcept { return *m_window; }

    Scene* CreateScene(std::string_view name);
    [[nodiscard]] Scene* GetScene(const std::string& name);
    [[nodiscard]] ScriptLoader GetScriptLoader(const std::string& name);

    [[nodiscard]] Scene* GetCurrentScene() const { return currentScene; }

    // can be null
    [[nodiscard]] GraphicsThread* GetTransferThread() const { return m_transferThread; }
    // can be null
    [[nodiscard]] GraphicsThread* GetComputeThread() const { return m_computeThread; }
    [[nodiscard]] GraphicsThread& GetGraphicsThread() const { return *m_graphicsThread; }

    Engine(const Engine &) = delete;
    Engine(Engine &&) = delete;
    Engine& operator=(const Engine &) = delete;
    Engine& operator=(Engine &&) = delete;
private:
    std::unordered_map<std::string, Scene*> m_scenes{};
    std::unordered_map<std::string, ScriptLoader> m_scriptsLoader{};
    
    Scene* currentScene = nullptr;
    ResourceManager* m_resourceManager{};
    ThreadPool* m_threadPool{};
    ShaderCache* m_shaderCache{};
    Renderer* m_renderer{};
    Window* m_window{};

    // some devices do not have a separate transfer queue
    GraphicsThread* m_transferThread{};
    // some devices do not have a separate compute queue
    GraphicsThread* m_computeThread{};
    GraphicsThread* m_graphicsThread{};

    std::mutex m_mutex{};

    void Init();
    void Run();
    void CleanUp();

    void LoadScriptFuncs();

    friend Renderer;

    friend void InitEngine();
    friend void CloseEngine();
};

[[nodiscard]] FORCE_INLINE inline ResourceManager& GetResourceManager() noexcept { return Engine::Get().GetResourceManager(); }
[[nodiscard]] FORCE_INLINE inline ThreadPool& GetThreadPool() noexcept { return Engine::Get().GetThreadPool(); }
[[nodiscard]] FORCE_INLINE inline ShaderCache& GetShaderCache() noexcept { return Engine::Get().GetShaderCache(); }
[[nodiscard]] FORCE_INLINE inline Renderer& GetRenderer() noexcept { return Engine::Get().GetRenderer(); }
[[nodiscard]] FORCE_INLINE inline Window& GetWindow() noexcept { return Engine::Get().GetWindow(); }