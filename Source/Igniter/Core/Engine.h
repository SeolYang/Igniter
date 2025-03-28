#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Render/Common.h"
#include "Igniter/D3D12/GpuSyncPoint.h"

namespace ig
{
    struct IgniterDesc
    {
        U32 WindowWidth, WindowHeight;
        std::string_view WindowTitle;
    };

    class Application;
    class Timer;
    class Window;
    class RenderContext;
    class ImGuiContext;
    class InputManager;
    class AssetManager;
    class World;
    class SceneProxy;
    class Renderer;
    class AudioSystem;

    class Engine final
    {
        friend class Application;

    public:
        Engine(const IgniterDesc& desc);
        ~Engine();

        Engine(const Engine&) = delete;
        Engine(Engine&&) noexcept = delete;
        Engine& operator=(const Engine&) = delete;
        Engine& operator=(Engine&&) noexcept = delete;

        [[nodiscard]] static tf::Executor& GetTaskExecutor();
        [[nodiscard]] static Timer& GetTimer();
        [[nodiscard]] static Window& GetWindow();
        [[nodiscard]] static InputManager& GetInputManager();
        [[nodiscard]] static AudioSystem& GetAudioSystem();
        [[nodiscard]] static RenderContext& GetRenderContext();
        [[nodiscard]] static AssetManager& GetAssetManager();
        [[nodiscard]] static ImGuiContext& GetImGuiContext();
        [[nodiscard]] static World& GetWorld();
        [[nodiscard]] static SceneProxy& GetSceneProxy();
        [[nodiscard]] static Renderer& GetRenderer();

        [[nodiscard]] bool IsValid() const { return this == instance; }
        bool static IsInitialized() { return instance != nullptr && instance->bInitialized; }

        static void Stop();

    private:
        int Execute(Application& application);
        tf::Task ScheduleRenderFrame(tf::Taskflow& frameTaskflow);

    private:
        static Engine* instance;
        bool bInitialized = false;
        bool bShouldExit = false;

        InFlightFramesResource<GpuSyncPoint> localFrameRenderSyncPoint{};
        LocalFrameIndex prevLocalFrameIdx = 0;

        tf::Executor taskExecutor{};
        Ptr<Timer> timer;
        Ptr<Window> window;
        Ptr<InputManager> inputManager;
        Ptr<AudioSystem> audioSystem;

        Ptr<RenderContext> renderContext;

        Ptr<AssetManager> assetManager;
        Ptr<ImGuiContext> imguiContext;

        Ptr<SceneProxy> sceneProxy;

        Ptr<Renderer> renderer;

        Ptr<World> world;
    };
} // namespace ig
