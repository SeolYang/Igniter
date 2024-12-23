#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/String.h"
#include "Igniter/D3D12/GpuSyncPoint.h"

namespace ig
{
    struct IgniterDesc
    {
        uint32_t WindowWidth, WindowHeight;
        String WindowTitle;
    };

    class Application;
    class Timer;
    class Window;
    class RenderContext;
    class InputManager;
    class AssetManager;
    class ImGuiContext;
    class ImGuiRenderer;
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
        [[nodiscard]] static RenderContext& GetRenderContext();
        [[nodiscard]] static AssetManager& GetAssetManager();
        [[nodiscard]] static ImGuiContext& GetImGuiContext();
        [[nodiscard]] static ImGuiRenderer& GetImGuiRenderer();

        [[nodiscard]] bool IsValid() const { return this == instance; }
        bool static IsInitialized() { return instance != nullptr && instance->bInitialized; }

        static void Stop();

    private:
        int Execute(Application& application);

    private:
        static Engine* instance;
        bool bInitialized = false;
        bool bShouldExit = false;

        eastl::array<GpuSyncPoint, NumFramesInFlight> localFrameSyncs{};

        /* L# stands for Dependency Level */
        //////////////////////// L0 ////////////////////////
        tf::Executor taskExecutor{};
        Ptr<Timer> timer;
        Ptr<Window> window;
        Ptr<InputManager> inputManager;
        ////////////////////////////////////////////////////

        //////////////////////// L1 ////////////////////////
        Ptr<RenderContext> renderContext;
        ////////////////////////////////////////////////////

        //////////////////////// L2 ////////////////////////
        Ptr<AssetManager> assetManager;
        Ptr<ImGuiContext> imguiContext;
        Ptr<ImGuiRenderer> imguiRenderer;
        ////////////////////////////////////////////////////
    };
}    // namespace ig
