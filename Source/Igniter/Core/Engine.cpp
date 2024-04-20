#include <Igniter.h>
#include <Core/Engine.h>
#include <Core/Assert.h>
#include <Core/Log.h>
#include <Core/Version.h>
#include <Core/FrameManager.h>
#include <Core/Timer.h>
#include <Core/HandleManager.h>
#include <Core/Window.h>
#include <Core/DeferredDeallocator.h>
#include <Core/EmbededSettings.h>
#include <Core/ComInitializer.h>
#include <Input/InputManager.h>
#include <D3D12/RenderDevice.h>
#include <Render/RenderContext.h>
#include <Render/GpuUploader.h>
#include <Render/GPUViewManager.h>
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
        CoInitializeUnique();
        IG_LOG(Engine, Info, "Igniter Engine Version {}", version::Version);
        IG_LOG(Engine, Info, "Igniting Engine Runtime!");
        //////////////////////// L0 ////////////////////////
        frameManager = MakePtr<FrameManager>();
        timer        = MakePtr<Timer>();
        /* #sy_test 임시 윈도우 설명자 */
        window = MakePtr<Window>(WindowDescription{
            .Width = 1920, .Height = 1080, .Title = String(settings::GameName)
        });
        renderDevice  = MakePtr<RenderDevice>();
        handleManager = MakePtr<HandleManager>();
        ////////////////////////////////////////////////////

        //////////////////////// L1 ////////////////////////
        inputManager        = MakePtr<InputManager>(*handleManager);
        deferredDeallocator = MakePtr<DeferredDeallocator>(*frameManager);
        ////////////////////////////////////////////////////

        //////////////////////// L2 ////////////////////////
        renderContext = MakePtr<RenderContext>(*deferredDeallocator, *renderDevice, *handleManager);
        ////////////////////////////////////////////////////

        //////////////////////// L3 ////////////////////////
        assetManager = MakePtr<AssetManager>(*handleManager, *renderDevice, *renderContext);
        /* #sy_test Temporary */
        ////////////////////////////////////////////////////

        //////////////////////// L4 ////////////////////////
        renderer = MakePtr<Renderer>(*frameManager, *window, *renderDevice, *handleManager,
                                              *renderContext);
        imguiRenderer = MakePtr<ImGuiRenderer>(*frameManager, *window, *renderDevice);
        ////////////////////////////////////////////////////

        //////////////////////// APP ///////////////////////
        imguiCanvas  = MakePtr<ImGuiCanvas>();
        gameInstance = MakePtr<GameInstance>();
        ////////////////////////////////////////////////////
    }

    Igniter::~Igniter()
    {
        IG_LOG(Engine, Info, "Extinguishing Engine Runtime.");
        //////////////////////// APP ///////////////////////
        gameInstance.reset();
        imguiCanvas.reset();
        ////////////////////////////////////////////////////

        /* #sy_note 각 계층에서 여전히 지연 해제하는 경우, 상위 계층을 해제하기 전에 중간에 수동으로 처리 해주어야 함. */
        deferredDeallocator->FlushAllFrames();

        //////////////////////// L4 ////////////////////////
        imguiRenderer.reset();
        renderer.reset();
        ////////////////////////////////////////////////////

        //////////////////////// L3 ////////////////////////
        assetManager.reset();
        ////////////////////////////////////////////////////

        deferredDeallocator->FlushAllFrames();

        //////////////////////// L2 ////////////////////////
        renderContext.reset();
        ////////////////////////////////////////////////////

        //////////////////////// L1 ////////////////////////
        deferredDeallocator.reset();
        inputManager.reset();
        ////////////////////////////////////////////////////

        //////////////////////// L0 ////////////////////////
        handleManager.reset();
        renderDevice.reset();
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

            deferredDeallocator->FlushCurrentFrame();
            assetManager->Update();

            gameInstance->Update();
            inputManager->PostUpdate();

            renderer->BeginFrame();
            {
                renderer->Render(gameInstance->GetRegistry());
                imguiRenderer->Render(*imguiCanvas, *renderer);
            }
            renderer->EndFrame();

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
        if (instance != nullptr)
        {
            IG_CHECK_NO_ENTRY();
            return;
        }

        instance = new Igniter();
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

    HandleManager& Igniter::GetHandleManager()
    {
        IG_CHECK(instance != nullptr);
        return *instance->handleManager;
    }

    Window& Igniter::GetWindow()
    {
        IG_CHECK(instance != nullptr);
        return *instance->window;
    }

    RenderDevice& Igniter::GetRenderDevice()
    {
        IG_CHECK(instance != nullptr);
        return *instance->renderDevice;
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

    DeferredDeallocator& Igniter::GetDeferredDeallocator()
    {
        IG_CHECK(instance != nullptr);
        return *instance->deferredDeallocator;
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
} // namespace ig
