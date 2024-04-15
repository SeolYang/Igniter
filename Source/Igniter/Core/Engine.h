#pragma once
#include <Igniter.h>

namespace ig
{
    class FrameManager;
    class Timer;
    class Window;
    class RenderDevice;
    class RenderContext;
    class GpuUploader;
    class HandleManager;
    class InputManager;
    class GpuViewManager;
    class DeferredDeallocator;
    class AssetManager;
    class Renderer;
    class ImGuiCanvas;
    class ImGuiRenderer;
    class GameInstance;

    class Igniter final
    {
    public:
        ~Igniter();

        Igniter(const Igniter&)                = delete;
        Igniter(Igniter&&) noexcept            = delete;
        Igniter& operator=(const Igniter&)     = delete;
        Igniter& operator=(Igniter&&) noexcept = delete;

        [[nodiscard]] static FrameManager&            GetFrameManager();
        [[nodiscard]] static Timer&                   GetTimer();
        [[nodiscard]] static Window&                  GetWindow();
        [[nodiscard]] static RenderDevice&            GetRenderDevice();
        [[nodiscard]] static HandleManager&           GetHandleManager();
        [[nodiscard]] static InputManager&            GetInputManager();
        [[nodiscard]] static DeferredDeallocator&     GetDeferredDeallocator();
        [[nodiscard]] static RenderContext&           GetRenderContext();
        [[nodiscard]] static AssetManager&            GetAssetManager();
        [[nodiscard]] static Renderer&                GetRenderer();
        [[nodiscard]] static ImGuiRenderer&           GetImGuiRenderer();
        [[nodiscard]] static ImGuiCanvas&             GetImGuiCanvas();
        [[nodiscard]] static GameInstance&            GetGameInstance();

        [[nodiscard]] bool IsValid() const { return this == instance; }

        bool static IsInitialized() { return instance != nullptr; }

        static void Ignite();
        static void Extinguish();
        static void Stop();
        static int Execute();

    private:
        Igniter();

        int ExecuteImpl();

    private:
        static Igniter* instance;
        bool            bShouldExit = false;

        /* L# stands for Dependency Level */
        //////////////////////// L0 ////////////////////////
        Ptr<FrameManager>  frameManager;
        Ptr<Timer>         timer;
        Ptr<Window>        window;
        Ptr<RenderDevice>  renderDevice;
        Ptr<HandleManager> handleManager;
        ////////////////////////////////////////////////////

        //////////////////////// L1 ////////////////////////
        Ptr<InputManager>        inputManager;
        Ptr<DeferredDeallocator> deferredDeallocator;
        ////////////////////////////////////////////////////

        //////////////////////// L2 ////////////////////////
        Ptr<RenderContext> renderContext;
        ////////////////////////////////////////////////////

        //////////////////////// L3 ////////////////////////
        Ptr<AssetManager> assetManager;
        ////////////////////////////////////////////////////

        //////////////////////// L4 ////////////////////////
        Ptr<Renderer>      renderer;
        Ptr<ImGuiRenderer> imguiRenderer;
        ////////////////////////////////////////////////////

        //////////////////////// APP ///////////////////////
        Ptr<ImGuiCanvas>  imguiCanvas;
        Ptr<GameInstance> gameInstance;
        ////////////////////////////////////////////////////
    };
} // namespace ig
