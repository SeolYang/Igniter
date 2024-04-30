#pragma once
#include <Igniter.h>
#include <Core/String.h>

namespace ig
{
    struct IgniterDesc
    {
        uint32_t WindowWidth, WindowHeight;
        String WindowTitle;
    };

    class Application;
    class FrameManager;
    class Timer;
    class Window;
    class RenderContext;
    class GpuUploader;
    class InputManager;
    class AssetManager;
    class Renderer;
    class ImGuiCanvas;
    class ImGuiRenderer;
    class GameInstance;
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

        [[nodiscard]] static FrameManager& GetFrameManager();
        [[nodiscard]] static Timer& GetTimer();
        [[nodiscard]] static Window& GetWindow();
        [[nodiscard]] static InputManager& GetInputManager();
        [[nodiscard]] static RenderContext& GetRenderContext();
        [[nodiscard]] static AssetManager& GetAssetManager();
        [[nodiscard]] static Renderer& GetRenderer();
        [[nodiscard]] static ImGuiRenderer& GetImGuiRenderer();

        [[nodiscard]] bool IsValid() const { return this == instance; }
        bool static IsInitialized() { return instance != nullptr && instance->bInitialized; }

        [[nodiscard]] static void SetImGuiCanvas(ImGuiCanvas* canvas);

        static void Stop();

    private:
        int Execute(Application& application);

    private:
        static Igniter* instance;
        bool bInitialized = false;
        bool bShouldExit = false;

        /* L# stands for Dependency Level */
        //////////////////////// L0 ////////////////////////
        Ptr<FrameManager> frameManager;
        Ptr<Timer> timer;
        Ptr<Window> window;
        Ptr<InputManager> inputManager;
        ////////////////////////////////////////////////////

        //////////////////////// L1 ////////////////////////
        Ptr<RenderContext> renderContext;
        ////////////////////////////////////////////////////

        //////////////////////// L2 ////////////////////////
        Ptr<AssetManager> assetManager;
        Ptr<Renderer> renderer;
        Ptr<ImGuiRenderer> imguiRenderer;
        ////////////////////////////////////////////////////

        //////////////////////// APP ///////////////////////
        ImGuiCanvas* imguiCanvas = nullptr;
        ////////////////////////////////////////////////////
    };
}    // namespace ig
