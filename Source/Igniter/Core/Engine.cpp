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
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Asset/AssetManager.h"
#include "Igniter/ImGui/ImGuiContext.h"
#include "Igniter/ImGui/ImGuiRenderer.h"
#include "Igniter/Application/Application.h"

IG_DEFINE_LOG_CATEGORY(Engine);

namespace ig
{
    Engine* Engine::instance = nullptr;

    Engine::Engine(const IgniterDesc& desc)
    {
        instance = this;
        CoInitializeUnique();
        IG_LOG(Engine, Info, "Igniter Engine Version {}", version::Version);
        IG_LOG(Engine, Info, "Igniting Engine Runtime!");
        //////////////////////// L0 ////////////////////////
        timer = MakePtr<Timer>();
        window = MakePtr<Window>(WindowDescription{.Width = desc.WindowWidth, .Height = desc.WindowHeight, .Title = desc.WindowTitle});
        inputManager = MakePtr<InputManager>();
        ////////////////////////////////////////////////////

        //////////////////////// L1 ////////////////////////
        renderContext = MakePtr<RenderContext>(*window);
        ////////////////////////////////////////////////////

        //////////////////////// L2 ////////////////////////
        assetManager = MakePtr<AssetManager>(*renderContext);
        assetManager->RegisterEngineDefault();
        imguiContext = MakePtr<ImGuiContext>(*window, *renderContext);
        imguiRenderer = MakePtr<ImGuiRenderer>(*renderContext);
        ////////////////////////////////////////////////////

        bInitialized = true;
    }

    Engine::~Engine()
    {
        IG_LOG(Engine, Info, "Extinguishing Engine Runtime.");

        //////////////////////// L2 ////////////////////////
        imguiContext.reset();
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
        ////////////////////////////////////////////////////

        IG_LOG(Engine, Info, "Engine Runtime Extinguished");
        if (instance == this)
        {
            instance = nullptr;
        }
    }

    int Engine::Execute(Application& application)
    {
        IG_LOG(Engine, Info, "Igniting Main Loop!");
        while (!bShouldExit)
        {
            ZoneScoped;
            /********  Begin Frame  *******/
            const LocalFrameIndex localFrameIdx = FrameManager::GetLocalFrameIndex();
            timer->Begin();

            /************* Event Handle/Proccess/Dispatch *************/
            {
                window->PumpMessage();
                if (bShouldExit)
                {
                    break;
                }

                inputManager->HandleRawMouseInput();
                assetManager->DispatchEvent();
            }

            const float deltaTime = timer->GetDeltaTime();
            /*********** Pre-Update **********/
            {
                application.PreUpdate(deltaTime);
            }

            /************* Update ************/
            {
                application.Update(deltaTime);
            }

            /*********** Post-Update **********/
            {
                inputManager->PostUpdate();
                application.PostUpdate(deltaTime);
            }

            /* Rendering: Wait for GPU */
            {
                localFrameSyncs[localFrameIdx].WaitOnCpu();
            }

            /*********** Pre-Render ***********/
            {
                renderContext->PreRender(localFrameIdx);
                application.PreRender(localFrameIdx);
            }

            {
                GpuSync mainRenderPipelineSyncPoint = application.Render(localFrameIdx);
                imguiRenderer->SetMainPipelineSyncPoint(mainRenderPipelineSyncPoint);
                localFrameSyncs[localFrameIdx] = imguiRenderer->Render(localFrameIdx);
            }

            /*********** Post-Render ***********/
            {
                application.PostRender(localFrameIdx);
                renderContext->PostRender(localFrameIdx);
            }

            /***********  End Frame  *************/
            timer->End();
            FrameManager::EndFrame();
        }

        renderContext->FlushQueues();
        IG_LOG(Engine, Info, "Extinguishing Engine Main Loop.");
        return 0;
    }

    void Engine::Stop()
    {
        IG_CHECK(instance != nullptr);
        instance->bShouldExit = true;
    }

    tf::Executor& Engine::GetTaskExecutor()
    {
        IG_CHECK(instance != nullptr);
        return instance->taskExecutor;
    }

    Timer& Engine::GetTimer()
    {
        IG_CHECK(instance != nullptr);
        return *instance->timer;
    }

    Window& Engine::GetWindow()
    {
        IG_CHECK(instance != nullptr);
        return *instance->window;
    }

    RenderContext& Engine::GetRenderContext()
    {
        IG_CHECK(instance != nullptr);
        return *instance->renderContext;
    }

    InputManager& Engine::GetInputManager()
    {
        IG_CHECK(instance != nullptr);
        return *instance->inputManager;
    }

    AssetManager& Engine::GetAssetManager()
    {
        IG_CHECK(instance != nullptr);
        return *instance->assetManager;
    }

    ImGuiContext& Engine::GetImGuiContext()
    {
        IG_CHECK(instance != nullptr);
        return *instance->imguiContext;
    }

    ImGuiRenderer& Engine::GetImGuiRenderer()
    {
        IG_CHECK(instance != nullptr);
        return *instance->imguiRenderer;
    }
}    // namespace ig
