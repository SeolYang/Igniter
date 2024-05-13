#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/String.h"
#include "Igniter/D3D12/GpuSync.h"

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
    class Igniter final
    {
        friend class Application;

    public:
        Igniter(const IgniterDesc& desc);
        ~Igniter();

        Igniter(const Igniter&) = delete;
        Igniter(Igniter&&) noexcept = delete;
        Igniter& operator=(const Igniter&) = delete;
        Igniter& operator=(Igniter&&) noexcept = delete;

        [[nodiscard]] static tf::Executor& GetTaskExecutor();
        [[nodiscard]] static Timer& GetTimer();
        [[nodiscard]] static Window& GetWindow();
        [[nodiscard]] static InputManager& GetInputManager();
        [[nodiscard]] static RenderContext& GetRenderContext();
        [[nodiscard]] static AssetManager& GetAssetManager();

        [[nodiscard]] bool IsValid() const { return this == instance; }
        bool static IsInitialized() { return instance != nullptr && instance->bInitialized; }

        static void Stop();

    private:
        int Execute(Application& application);

    private:
        static Igniter* instance;
        bool bInitialized = false;
        bool bShouldExit = false;

        eastl::array<GpuSync, NumFramesInFlight> localFrameSyncs{};

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
        ////////////////////////////////////////////////////
    };
}    // namespace ig
