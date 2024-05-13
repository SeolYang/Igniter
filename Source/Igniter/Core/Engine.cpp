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
        ////////////////////////////////////////////////////

        bInitialized = true;
    }

    Igniter::~Igniter()
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

    int Igniter::Execute(Application& application)
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

            /*********** Pre-Render ***********/
            {
                localFrameSyncs[localFrameIdx].WaitOnCpu();
                renderContext->PreRender(localFrameIdx);
                application.PreRender(localFrameIdx);
                // imguiRenderer->SetImGuiCanvas(imguiCanvas);
            }

            /************* Render *************/
            {
                localFrameSyncs[localFrameIdx] = application.Render(localFrameIdx);
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

    void Igniter::Stop()
    {
        IG_CHECK(instance != nullptr);
        instance->bShouldExit = true;
    }

    tf::Executor& Igniter::GetTaskExecutor()
    {
        IG_CHECK(instance != nullptr);
        return instance->taskExecutor;
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

    AssetManager& Igniter::GetAssetManager()
    {
        IG_CHECK(instance != nullptr);
        return *instance->assetManager;
    }
}    // namespace ig
