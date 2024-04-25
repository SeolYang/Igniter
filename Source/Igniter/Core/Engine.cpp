#include <Igniter.h>
#include <Core/Engine.h>
#include <Core/Assert.h>
#include <Core/Log.h>
#include <Core/Version.h>
#include <Core/FrameManager.h>
#include <Core/Timer.h>
#include <Core/Window.h>
#include <Core/EmbededSettings.h>
#include <Core/ComInitializer.h>
#include <Input/InputManager.h>
#include <D3D12/RenderDevice.h>
#include <Render/RenderContext.h>
#include <Render/GpuUploader.h>
#include <Render/Renderer.h>
#include <ImGui/ImGuiRenderer.h>
#include <ImGui/ImGuiCanvas.h>
#include <Asset/AssetManager.h>
#include <Gameplay/GameInstance.h>

IG_DEFINE_LOG_CATEGORY(Engine);

namespace ig
{
    Igniter* Igniter::instance = nullptr;

    Igniter::Igniter()
    {
        instance = this;
        CoInitializeUnique();
        IG_LOG(Engine, Info, "Igniter Engine Version {}", version::Version);
        IG_LOG(Engine, Info, "Igniting Engine Runtime!");
        //////////////////////// L0 ////////////////////////
        frameManager = MakePtr<FrameManager>();
        timer = MakePtr<Timer>();
        /* #sy_test 임시 윈도우 설명자 */
        window = MakePtr<Window>(WindowDescription{.Width = 1920, .Height = 1080, .Title = String(settings::GameName)});
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

        //////////////////////// APP ///////////////////////
        imguiCanvas = MakePtr<ImGuiCanvas>();
        gameInstance = MakePtr<GameInstance>();
        ////////////////////////////////////////////////////

        bInitialized = true;
    }

    Igniter::~Igniter()
    {
        IG_LOG(Engine, Info, "Extinguishing Engine Runtime.");
        //////////////////////// APP ///////////////////////
        gameInstance.reset();
        imguiCanvas.reset();
        ////////////////////////////////////////////////////

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

    int Igniter::ExecuteImpl()
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

            assetManager->Update();

            gameInstance->Update();
            inputManager->PostUpdate();

            const uint8_t localFrameIdx = frameManager->GetLocalFrameIndex();
            renderer->BeginFrame(localFrameIdx);
            renderContext->BeginFrame(localFrameIdx);
            {
                renderer->Render(localFrameIdx, gameInstance->GetRegistry());
                imguiRenderer->Render(*imguiCanvas, *renderer);
            }
            renderer->EndFrame(localFrameIdx);

            timer->End();
            frameManager->NextFrame();
        }

        renderContext->FlushQueues();

        IG_LOG(Engine, Info, "Extinguishing Engine Main Loop.");
        return 0;
    }

    int Igniter::Execute()
    {
        IG_CHECK(instance != nullptr);
        return instance->ExecuteImpl();
    }

    void Igniter::Stop()
    {
        IG_CHECK(instance != nullptr);
        instance->bShouldExit = true;
    }

    void Igniter::Ignite()
    {
        /* 좀 더 나은 방식 찾아보기 */
        if (instance != nullptr)
        {
            IG_CHECK_NO_ENTRY();
            return;
        }

        [[maybe_unused]] Igniter* newInstance = new Igniter();
        IG_CHECK(newInstance == instance);
    }

    void Igniter::Extinguish()
    {
        if (instance == nullptr)
        {
            IG_CHECK_NO_ENTRY();
            return;
        }

        delete instance;
        instance = nullptr;
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

    ImGuiCanvas& Igniter::GetImGuiCanvas()
    {
        IG_CHECK(instance != nullptr);
        return *instance->imguiCanvas;
    }

    GameInstance& Igniter::GetGameInstance()
    {
        IG_CHECK(instance != nullptr);
        return *(instance->gameInstance);
    }
}    // namespace ig
