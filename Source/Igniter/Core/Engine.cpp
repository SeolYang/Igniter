#include "Igniter/Igniter.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Core/Assert.h"
#include "Igniter/Core/Log.h"
#include "Igniter/Core/Version.h"
#include "Igniter/Core/FrameManager.h"
#include "Igniter/Core/Timer.h"
#include "Igniter/Core/Window.h"
#include "Igniter/Core/EmbededSettings.h"
#include "Igniter/Core/ComInitializer.h"
#include "Igniter/Input/InputManager.h"
#include "Igniter/D3D12/RenderDevice.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/GpuUploader.h"
#include "Igniter/Render/Renderer.h"
#include "Igniter/ImGui/ImGuiRenderer.h"
#include "Igniter/ImGui/ImGuiCanvas.h"
#include "Igniter/Asset/AssetManager.h"
#include "Igniter/Application/Application.h"

IG_DEFINE_LOG_CATEGORY(Engine);

namespace ig
{
    Igniter* Igniter::instance = nullptr;

    Igniter::Igniter(const IgniterDesc& desc)
    {
        instance = this;
        CoInitializeUnique();
        IG_LOG(Engine, Info, "Igniter Engine Version {}", version::Version);
        IG_LOG(Engine, Info, "Igniting Engine Runtime!");
        //////////////////////// L0 ////////////////////////
        frameManager = MakePtr<FrameManager>();
        timer = MakePtr<Timer>();
        window = MakePtr<Window>(WindowDescription{.Width = desc.WindowWidth, .Height = desc.WindowHeight, .Title = desc.WindowTitle});
        inputManager = MakePtr<InputManager>();
        ////////////////////////////////////////////////////

        //////////////////////// L1 ////////////////////////
        renderContext = MakePtr<RenderContext>(*frameManager);
        ////////////////////////////////////////////////////

        //////////////////////// L2 ////////////////////////
        assetManager = MakePtr<AssetManager>(*renderContext);
        assetManager->RegisterEngineDefault();
        renderer = MakePtr<Renderer>(*frameManager, *window, *renderContext);
        imguiRenderer = MakePtr<ImGuiRenderer>(*frameManager, *window, *renderContext);
        ////////////////////////////////////////////////////

        bInitialized = true;
    }

    Igniter::~Igniter()
    {
        IG_LOG(Engine, Info, "Extinguishing Engine Runtime.");

        //////////////////////// L2 ////////////////////////
        imguiRenderer.reset();
        renderer.reset();
        assetManager->UnRegisterEngineDefault();
        assetManager.reset();
        ////////////////////////////////////////////////////

        //////////////////////// L1 ////////////////////////
        renderContext.reset();
        ////////////////////////////////////////////////////

        //////////////////////// L0 ////////////////////////
        inputManager.reset();
        window.reset();
        timer.reset();
        frameManager.reset();
        ////////////////////////////////////////////////////

        IG_LOG(Engine, Info, "Engine Runtime Extinguished");
        if (instance == this)
        {
            instance = nullptr;
        }
    }

    int Igniter::Execute(Application& application)
    {
        IG_LOG(Engine, Info, "Igniting Main Loop!");
        while (!bShouldExit)
        {
            ZoneScoped;
            timer->Begin();

            MSG msg;
            ZeroMemory(&msg, sizeof(msg));
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                {
                    bShouldExit = true;
                }

                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            const float deltaTime = timer->GetDeltaTime();

            {
                assetManager->Update();
                application.Update(deltaTime);
            }

            {
                inputManager->PostUpdate();
            }

            {
                const uint8_t localFrameIdx = frameManager->GetLocalFrameIndex();
                renderer->BeginFrame(localFrameIdx);
                renderContext->BeginFrame(localFrameIdx);
                {
                    application.Render(localFrameIdx);
                    imguiRenderer->Render(imguiCanvas, *renderer);
                }
                renderer->EndFrame(localFrameIdx);
            }

            timer->End();
            frameManager->NextFrame();
        }

        renderContext->FlushQueues();
        IG_LOG(Engine, Info, "Extinguishing Engine Main Loop.");
        return 0;
    }

    void Igniter::SetImGuiCanvas(ImGuiCanvas* canvas)
    {
        if (instance != nullptr)
        {
            instance->imguiCanvas = canvas;
        }
    }

    void Igniter::Stop()
    {
        IG_CHECK(instance != nullptr);
        instance->bShouldExit = true;
    }

    FrameManager& Igniter::GetFrameManager()
    {
        IG_CHECK(instance != nullptr);
        return *instance->frameManager;
    }

    Timer& Igniter::GetTimer()
    {
        IG_CHECK(instance != nullptr);
        return *instance->timer;
    }

    Window& Igniter::GetWindow()
    {
        IG_CHECK(instance != nullptr);
        return *instance->window;
    }

    RenderContext& Igniter::GetRenderContext()
    {
        IG_CHECK(instance != nullptr);
        return *instance->renderContext;
    }

    InputManager& Igniter::GetInputManager()
    {
        IG_CHECK(instance != nullptr);
        return *instance->inputManager;
    }

    Renderer& Igniter::GetRenderer()
    {
        IG_CHECK(instance != nullptr);
        return *instance->renderer;
    }

    ImGuiRenderer& Igniter::GetImGuiRenderer()
    {
        IG_CHECK(instance != nullptr);
        return *instance->imguiRenderer;
    }

    AssetManager& Igniter::GetAssetManager()
    {
        IG_CHECK(instance != nullptr);
        return *instance->assetManager;
    }
}    // namespace ig
