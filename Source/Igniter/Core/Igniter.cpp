#include <Core/Igniter.h>
#include <Core/Assert.h>
#include <Core/Log.h>
#include <Core/HandleManager.h>
#include <Core/InputManager.h>
#include <Core/Window.h>
#include <Core/DeferredDeallocator.h>
#include <Core/EmbededSettings.h>
#include <D3D12/RenderDevice.h>
#include <Render/GpuUploader.h>
#include <Render/GPUViewManager.h>
#include <Render/Renderer.h>
#include <ImGui/ImGuiRenderer.h>
#include <ImGui/ImGuiCanvas.h>
#include <Gameplay/GameInstance.h>

namespace ig
{
    IG_DEFINE_LOG_CATEGORY(EngineInfo, ELogVerbosity::Info)
    IG_DEFINE_LOG_CATEGORY(EngineFatal, ELogVerbosity::Fatal)

    Igniter* Igniter::instance = nullptr;
    Igniter::Igniter()
    {
        const bool bIsFirstEngineCreation = instance == nullptr;
        IG_CHECK(bIsFirstEngineCreation);
        if (bIsFirstEngineCreation)
        {
            instance = this;
            logger = std::make_unique<Logger>();
            logger->Log<EngineInfo>("Igniter Engine version: {}", version::Version);
            logger->Log<EngineInfo>("Igniting Engine Runtime!");

            /* #sy_test 임시 윈도우 설명자 */
            const WindowDescription winDesc{ .Width = 1920, .Height = 1080, .Title = String(settings::GameName) };
            window = std::make_unique<Window>(winDesc);
            renderDevice = std::make_unique<RenderDevice>();
            gpuUploader = std::make_unique<GpuUploader>(*renderDevice);
            handleManager = std::make_unique<HandleManager>();
            deferredDeallocator = std::make_unique<DeferredDeallocator>(frameManager);
            inputManager = std::make_unique<InputManager>(*handleManager);
            gpuViewManager = std::make_unique<GpuViewManager>(*handleManager, *deferredDeallocator, *renderDevice);
            renderer = std::make_unique<Renderer>(frameManager, *deferredDeallocator, *window, *renderDevice, *handleManager, *gpuViewManager);
            imguiRenderer = std::make_unique<ImGuiRenderer>(frameManager, *window, *renderDevice);
            imguiCanvas = std::make_unique<ImGuiCanvas>();
            gameInstance = std::make_unique<GameInstance>();
        }
    }

    Igniter::~Igniter()
    {
        logger->Log<EngineInfo>("* Cleanup sub-systems");
        gpuViewManager->ClearCachedSampler();
        inputManager->Clear();
        deferredDeallocator->FlushAllFrames();

        gameInstance.reset();
        imguiCanvas.reset();
        imguiRenderer.reset();
        renderer.reset();
        inputManager.reset();
        deferredDeallocator.reset();
        gpuViewManager.reset();
        handleManager.reset();
        gpuUploader.reset();
        renderDevice.reset();
        window.reset();
        logger.reset();

        if (instance == this)
        {
            instance = nullptr;
        }

        String::ClearCache();
    }

    FrameManager& Igniter::GetFrameManager()
    {
        IG_CHECK(instance != nullptr);
        return instance->frameManager;
    }

    Timer& Igniter::GetTimer()
    {
        IG_CHECK(instance != nullptr);
        return instance->timer;
    }

    Logger& Igniter::GetLogger()
    {
        IG_CHECK(instance != nullptr);
        return *instance->logger;
    }

    HandleManager& Igniter::GetHandleManager()
    {
        IG_CHECK(instance != nullptr);
        return *(instance->handleManager);
    }

    Window& Igniter::GetWindow()
    {
        IG_CHECK(instance != nullptr);
        return *(instance->window);
    }

    RenderDevice& Igniter::GetRenderDevice()
    {
        IG_CHECK(instance != nullptr);
        return *(instance->renderDevice);
    }

    GpuUploader& Igniter::GetGpuUploader()
    {
        IG_CHECK(instance != nullptr);
        return *(instance->gpuUploader);
    }

    InputManager& Igniter::GetInputManager()
    {
        IG_CHECK(instance != nullptr);
        return *(instance->inputManager);
    }

    DeferredDeallocator& Igniter::GetDeferredDeallocator()
    {
        IG_CHECK(instance != nullptr);
        return *(instance->deferredDeallocator);
    }

    GpuViewManager& Igniter::GetGPUViewManager()
    {
        IG_CHECK(instance != nullptr);
        return *(instance->gpuViewManager);
    }

    Renderer& Igniter::GetRenderer()
    {
        IG_CHECK(instance != nullptr);
        return *(instance->renderer);
    }

    ImGuiRenderer& Igniter::GetImGuiRenderer()
    {
        IG_CHECK(instance != nullptr);
        return *(instance->imguiRenderer);
    }

    ImGuiCanvas& Igniter::GetImGuiCanvas()
    {
        IG_CHECK(instance != nullptr);
        return *(instance->imguiCanvas);
    }

    GameInstance& Igniter::GetGameInstance()
    {
        IG_CHECK(instance != nullptr);
        return *(instance->gameInstance);
    }

    void Igniter::Exit()
    {
        IG_CHECK(instance != nullptr);
        instance->bShouldExit = true;
    }

    int Igniter::Execute()
    {
        logger->Log<EngineInfo>("* Start Engine main loop");

        while (!bShouldExit)
        {
            timer.Begin();

            MSG msg;
            ZeroMemory(&msg, sizeof(msg));
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                {
                    bShouldExit = true;
                }

                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            deferredDeallocator->FlushCurrentFrame();

            gameInstance->Update();
            inputManager->PostUpdate();

            renderer->BeginFrame();
            {
                renderer->Render(gameInstance->GetRegistry());
                imguiRenderer->Render(*imguiCanvas, *renderer);
            }
            renderer->EndFrame();

            timer.End();
            frameManager.NextFrame();
        }

        renderer->FlushQueues();
        logger->Log<EngineInfo>("* End Engine main loop");
        return 0;
    }
} // namespace ig
