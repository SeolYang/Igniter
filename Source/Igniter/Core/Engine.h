#pragma once
#include <Igniter.h>

namespace ig
{
    class FrameManager;
    class Timer;
    class Window;
    class RenderDevice;
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
        Igniter();
        ~Igniter();

        [[nodiscard]] static FrameManager& GetFrameManager();
        [[nodiscard]] static Timer& GetTimer();
        [[nodiscard]] static Window& GetWindow();
        [[nodiscard]] static RenderDevice& GetRenderDevice();
        [[nodiscard]] static GpuUploader& GetGpuUploader();
        [[nodiscard]] static HandleManager& GetHandleManager();

        [[nodiscard]] static InputManager& GetInputManager();
        [[nodiscard]] static DeferredDeallocator& GetDeferredDeallocator();
        [[nodiscard]] static GpuViewManager& GetGPUViewManager();
        [[nodiscard]] static ImGuiRenderer& GetImGuiRenderer();

        [[nodiscard]] static AssetManager& GetAssetManager();
        [[nodiscard]] static Renderer& GetRenderer();

        [[nodiscard]] static ImGuiCanvas& GetImGuiCanvas();
        [[nodiscard]] static OptionalRef<ImGuiCanvas> TryGetImGuiCanvas();
        [[nodiscard]] static GameInstance& GetGameInstance();

        bool IsValid() const { return this == instance; }

        bool static IsInitialized() { return instance != nullptr; }

        [[nodiscard]] static void Exit();

        int Execute();

    private:
        static Igniter* instance;
        bool bShouldExit = false;

        /* L# stand for Dependency Level */
        //////////////////////// L0 ////////////////////////
        std::unique_ptr<FrameManager> frameManager;
        std::unique_ptr<Timer> timer;
        std::unique_ptr<Window> window;
        std::unique_ptr<RenderDevice> renderDevice;
        std::unique_ptr<HandleManager> handleManager;
        ////////////////////////////////////////////////////

        //////////////////////// L1 ////////////////////////
        std::unique_ptr<InputManager> inputManager;
        std::unique_ptr<DeferredDeallocator> deferredDeallocator;
        std::unique_ptr<GpuUploader> gpuUploader;
        std::unique_ptr<ImGuiRenderer> imguiRenderer;
        ////////////////////////////////////////////////////

        //////////////////////// L2 ////////////////////////
        std::unique_ptr<GpuViewManager> gpuViewManager;
        ////////////////////////////////////////////////////

        //////////////////////// L3 ////////////////////////
        std::unique_ptr<AssetManager> assetManager;
        std::unique_ptr<Renderer> renderer;
        ////////////////////////////////////////////////////

        //////////////////////// APP ///////////////////////
        std::unique_ptr<ImGuiCanvas> imguiCanvas;
        std::unique_ptr<GameInstance> gameInstance;
        ////////////////////////////////////////////////////
    };
} // namespace ig
