#pragma once
#include <Igniter.h>
#include <Core/Version.h>
#include <Core/FrameManager.h>
#include <Core/Timer.h>
#include <Core/Container.h>

namespace ig
{
    class Window;
    class RenderDevice;
    class GpuUploader;
    class HandleManager;
    class InputManager;
    class GpuViewManager;
    class DeferredDeallocator;
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
        [[nodiscard]] static Renderer& GetRenderer();
        [[nodiscard]] static ImGuiRenderer& GetImGuiRenderer();
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
        FrameManager frameManager;
        Timer timer;
        std::unique_ptr<Window> window;
        std::unique_ptr<RenderDevice> renderDevice;
        std::unique_ptr<GpuUploader> gpuUploader;
        std::unique_ptr<HandleManager> handleManager;
        std::unique_ptr<DeferredDeallocator> deferredDeallocator;
        std::unique_ptr<InputManager> inputManager;
        std::unique_ptr<GpuViewManager> gpuViewManager;
        std::unique_ptr<Renderer> renderer;
        std::unique_ptr<ImGuiRenderer> imguiRenderer;
        std::unique_ptr<ImGuiCanvas> imguiCanvas;
        std::unique_ptr<GameInstance> gameInstance;
    };
} // namespace ig
