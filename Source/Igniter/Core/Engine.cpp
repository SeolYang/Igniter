#include "Igniter/Igniter.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Core/Assert.h"
#include "Igniter/Core/Log.h"
#include "Igniter/Core/Version.h"
#include "Igniter/Core/FrameManager.h"
#include "Igniter/Core/Timer.h"
#include "Igniter/Core/Window.h"
#include "Igniter/Core/ComInitializer.h"
#include "Igniter/Core/Thread.h"
#include "Igniter/Input/InputManager.h"
#include "Igniter/Audio/AudioSystem.h"
#include "Igniter/D3D12/GpuSyncPoint.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/SceneProxy.h"
#include "Igniter/Render/Renderer.h"
#include "Igniter/Asset/AssetManager.h"
#include "Igniter/ImGui/ImGuiContext.h"
#include "Igniter/Application/Application.h"
#include "Igniter/Gameplay/World.h"

IG_DECLARE_LOG_CATEGORY(EngineLog);

IG_DEFINE_LOG_CATEGORY(EngineLog);

namespace ig
{
    Engine* Engine::instance = nullptr;

    Engine::Engine(const IgniterDesc& desc)
        :
#if defined(DEBUG) || defined(_DEBUG)
        taskExecutor(2)
#else
        taskExecutor(std::thread::hardware_concurrency())
#endif
    {
        IG_CHECK(instance == nullptr);

        instance = this;
        IG_LOG(EngineLog, Info, "Igniter Engine Version {}", version::Version);
        IG_LOG(EngineLog, Info, "Igniting Engine Runtime!");
        CoInitializeUnique();
        // 엔진 인스턴스가 생성된 스레드를 메인 스레드로 가정
        ThreadInfo::RegisterMainThreadID();

        timer = MakePtr<Timer>();
        IG_LOG(EngineLog, Info, "Timer Initialized.");
        window = MakePtr<Window>(WindowDescription{.Width = desc.WindowWidth, .Height = desc.WindowHeight, .Title = desc.WindowTitle});
        IG_LOG(EngineLog, Info, "Window Initialized.");
        inputManager = MakePtr<InputManager>();
        IG_LOG(EngineLog, Info, "Input Manager Initialized.");

        audioSystem = MakePtr<AudioSystem>();
        IG_LOG(EngineLog, Info, "Audio System Initialized.");

        renderContext = MakePtr<RenderContext>(*window);
        IG_LOG(EngineLog, Info, "OnImGui Context Initialized.");

        assetManager = MakePtr<AssetManager>(*renderContext, *audioSystem);
        IG_LOG(EngineLog, Info, "Asset Manager Initialized.");
        imguiContext = MakePtr<ImGuiContext>(*window, *renderContext);
        IG_LOG(EngineLog, Info, "ImGui Context Initialized.");

        sceneProxy = MakePtr<SceneProxy>(taskExecutor, *renderContext, *assetManager);
        IG_LOG(EngineLog, Info, "Scene Proxy Initialized.");

        renderer = MakePtr<Renderer>(*window, *renderContext, *sceneProxy);
        IG_LOG(EngineLog, Info, "Renderer Initialized.");

        world = MakePtr<World>();
        IG_LOG(EngineLog, Info, "Empty World Initialized.");

        bInitialized = true;
        IG_LOG(EngineLog, Info, "Igniter Engine {} Initialized.", version::Version);
    }

    Engine::~Engine()
    {
        IG_LOG(EngineLog, Info, "Extinguishing Engine Runtime.");

        world.reset();
        IG_LOG(EngineLog, Info, "World Deinitialized.");

        renderer.reset();
        IG_LOG(EngineLog, Info, "Renderer Deinitialized.");

        sceneProxy.reset();
        IG_LOG(EngineLog, Info, "Scene Proxy Deinitialized.");

        imguiContext.reset();
        IG_LOG(EngineLog, Info, "ImGui Context Deinitialized.");
        assetManager.reset();
        IG_LOG(EngineLog, Info, "Asset Manager Deinitialized.");

        renderContext.reset();
        IG_LOG(EngineLog, Info, "Render Context Deinitialized.");

        audioSystem.reset();
        IG_LOG(EngineLog, Info, "Audio System Deinitialized.");

        inputManager.reset();
        IG_LOG(EngineLog, Info, "Input Manager Deinitialized.");
        window.reset();
        IG_LOG(EngineLog, Info, "Window Instance Deinitialized.");
        timer.reset();

        IG_LOG(EngineLog, Info, "Engine Runtime Extinguished");
        IG_CHECK(instance == this);
        instance = nullptr;
    }

    int Engine::Execute(Application& application)
    {
        IG_LOG(EngineLog, Info, "Igniting Main Loop!");
        while (!bShouldExit)
        {
            FrameMark;
            ZoneScoped;
            /********  Begin Frame  *******/
            const LocalFrameIndex localFrameIdx = FrameManager::GetLocalFrameIndex();
            timer->Begin();
            const F32 deltaTime = timer->GetDeltaTime();

            /* OS Event 처리는 메인 스레드에서 이루어져야 한다. */
            {
                ZoneScopedN("Engine.Process Events");
                window->PumpMessage();
                if (bShouldExit)
                {
                    break;
                }

                inputManager->HandleRawMouseInput();
                assetManager->DispatchEvent();
            }

            {
                ZoneScopedN("Engine.Update");
                application.Update(deltaTime);
            }

            {
                ZoneScopedN("Engine.PostUpdate");
                inputManager->PostUpdate();
            }

            {
                ZoneScopedN("Engine.OnImGui");
                application.OnImGui();
            }

            const GlobalFrameIndex globalFrameIdx = FrameManager::GetGlobalFrameIndex();
            tf::Taskflow frameTaskflow{std::format("Frame#{}", globalFrameIdx)};
            [[maybe_unused]] tf::Task finalizeRenderFrameTask = ScheduleRenderFrame(frameTaskflow);
            tf::Task audioUpdateTask = frameTaskflow.emplace([this, deltaTime]()
            {
               audioSystem->Update(this->world->GetRegistry(), deltaTime); 
            });
            taskExecutor.run(frameTaskflow).wait();

            prevLocalFrameIdx = localFrameIdx;
            timer->End();
            FrameManager::EndFrame();
        }

        renderContext->FlushQueues();
        IG_LOG(EngineLog, Info, "Extinguishing Engine Main Loop.");
        return 0;
    }

    tf::Task Engine::ScheduleRenderFrame(tf::Taskflow& frameTaskflow)
    {
        const LocalFrameIndex localFrameIdx = FrameManager::GetLocalFrameIndex();

        tf::Task waitForLocalFrameTask = frameTaskflow.emplace([this, localFrameIdx]()
        {
            ZoneScopedN("Engine.WaitForLocalFrame");
            localFrameRenderSyncPoint[localFrameIdx].WaitOnCpu();
        }).name("Engine.WaitForLocalFrame");

        tf::Task preRenderTask = frameTaskflow.emplace([this, localFrameIdx]()
        {
            ZoneScopedN("Engine.PreRender");
            renderContext->PreRender(localFrameIdx, localFrameRenderSyncPoint[prevLocalFrameIdx]);
            renderer->PreRender(localFrameIdx);
        }).name("Engine.PreRender");

        tf::Task beginRenderTask = frameTaskflow.emplace([]() {}).name("Engine.BeginRender");

        tf::Task replicatateSceneProxyTask = frameTaskflow.emplace([this, localFrameIdx](tf::Subflow& replicationSubflow)
        {
            ZoneScopedN("Engine.ReplicateScene");
            sceneProxy->Replicate(replicationSubflow, localFrameIdx, *world);
            replicationSubflow.join();
            
            /* 비동기적으로 실행 */
            sceneProxy->PrepareNextFrame(localFrameIdx);
        }).name("Engine.ReplicateSceneProxy");

        preRenderTask.succeed(waitForLocalFrameTask);
        replicatateSceneProxyTask.succeed(preRenderTask);
        beginRenderTask.succeed(preRenderTask);

        tf::Task finalizeRenderTask = renderer->ScheduleRenderTasks(frameTaskflow,
            RenderFrameSchedulingParams{.localFrameIdx = localFrameIdx, .BeginRender = beginRenderTask, .SceneProxyReplication = replicatateSceneProxyTask});

        tf::Task finalizeRenderFrameTask = frameTaskflow.emplace([this, localFrameIdx]()
        {
            localFrameRenderSyncPoint[localFrameIdx] = renderer->GetLocalFrarmeRenderSyncPoint();
        }).name("Engine.FinalizeRenderFrame");
        finalizeRenderFrameTask.succeed(finalizeRenderTask);

        return finalizeRenderFrameTask;
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

    AudioSystem& Engine::GetAudioSystem()
    {
        IG_CHECK(instance != nullptr);
        return *instance->audioSystem;
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

    World& Engine::GetWorld()
    {
        IG_CHECK(instance != nullptr);
        return *instance->world;
    }

    SceneProxy& Engine::GetSceneProxy()
    {
        IG_CHECK(instance != nullptr);
        return *instance->sceneProxy;
    }

    Renderer& Engine::GetRenderer()
    {
        IG_CHECK(instance != nullptr);
        return *instance->renderer;
    }
} // namespace ig
