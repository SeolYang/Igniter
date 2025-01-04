#include "Igniter/Igniter.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Core/Assert.h"
#include "Igniter/Core/Log.h"
#include "Igniter/Core/Version.h"
#include "Igniter/Core/FrameManager.h"
#include "Igniter/Core/Timer.h"
#include "Igniter/Core/Window.h"
#include "Igniter/Core/ComInitializer.h"
#include "Igniter/Input/InputManager.h"
#include "Igniter/D3D12/GpuSyncPoint.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/MeshStorage.h"
#include "Igniter/Render/SceneProxy.h"
#include "Igniter/Render/Renderer.h"
#include "Igniter/Asset/AssetManager.h"
#include "Igniter/ImGui/ImGuiContext.h"
#include "Igniter/ImGui/ImGuiRenderer.h"
#include "Igniter/Application/Application.h"
#include "Igniter/Gameplay/World.h"

IG_DEFINE_LOG_CATEGORY(Engine);

namespace ig
{
    Engine* Engine::instance = nullptr;

    Engine::Engine(const IgniterDesc& desc)
    {
        IG_CHECK(instance == nullptr);

        instance = this;
        IG_LOG(Engine, Info, "Igniter Engine Version {}", version::Version);
        IG_LOG(Engine, Info, "Igniting Engine Runtime!");
        CoInitializeUnique();

        timer = MakePtr<Timer>();
        IG_LOG(Engine, Info, "Timer Initialized.");
        window = MakePtr<Window>(WindowDescription{.Width = desc.WindowWidth, .Height = desc.WindowHeight, .Title = desc.WindowTitle});
        IG_LOG(Engine, Info, "Window Initialized.");
        inputManager = MakePtr<InputManager>();
        IG_LOG(Engine, Info, "Input Manager Initialized.");

        renderContext = MakePtr<RenderContext>(*window);
        IG_LOG(Engine, Info, "Render Context Initialized.");

        meshStorage = MakePtr<MeshStorage>(*renderContext);
        IG_LOG(Engine, Info, "Mesh Storage Initialized.");

        assetManager = MakePtr<AssetManager>(*renderContext);
        IG_LOG(Engine, Info, "Asset Manager Initialized.");
        imguiContext = MakePtr<ImGuiContext>(*window, *renderContext);
        IG_LOG(Engine, Info, "ImGui Context Initialized.");
        imguiRenderer = MakePtr<ImGuiRenderer>(*renderContext);
        IG_LOG(Engine, Info, "ImGui Renderer Initialized.");

        sceneProxy = MakePtr<SceneProxy>(*renderContext, *meshStorage, *assetManager);
        IG_LOG(Engine, Info, "Scene Proxy Initialized.");

        renderer = MakePtr<Renderer>(*window, *renderContext, *meshStorage, *sceneProxy);
        IG_LOG(Engine, Info, "Renderer Initialized.");

        world = MakePtr<World>();
        IG_LOG(Engine, Info, "Empty World Initialized.");

        bInitialized = true;
        IG_LOG(Engine, Info, "Igniter Engine {} Initialized.", version::Version);
    }

    Engine::~Engine()
    {
        IG_LOG(Engine, Info, "Extinguishing Engine Runtime.");

        world.reset();
        IG_LOG(Engine, Info, "World Deinitialized.");

        renderer.reset();
        IG_LOG(Engine, Info, "Renderer Deinitialized.");

        sceneProxy.reset();
        IG_LOG(Engine, Info, "Scene Proxy Deinitialized.");

        imguiContext.reset();
        IG_LOG(Engine, Info, "ImGui Context Deinitialized.");
        assetManager.reset();
        IG_LOG(Engine, Info, "Asset Manager Deinitialized.");

        meshStorage.reset();
        IG_LOG(Engine, Info, "Mesh Storage Deinitialized.");

        renderContext.reset();
        IG_LOG(Engine, Info, "Render Context Deinitialized.");

        inputManager.reset();
        IG_LOG(Engine, Info, "Input Manager Deinitialized.");
        window.reset();
        IG_LOG(Engine, Info, "Window Instance Deinitialized.");
        timer.reset();

        IG_LOG(Engine, Info, "Engine Runtime Extinguished");
        IG_CHECK(instance == this);
        instance = nullptr;
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
                meshStorage->PreRender(localFrameIdx);
                sceneProxy->Replicate(localFrameIdx, *world);
                renderer->PreRender(localFrameIdx);
            }

            {
                GpuSyncPoint mainRenderPipelineSyncPoint = renderer->Render(localFrameIdx);
                imguiRenderer->SetMainPipelineSyncPoint(mainRenderPipelineSyncPoint);
                localFrameSyncs[localFrameIdx] = imguiRenderer->Render(localFrameIdx);
            }

            /*********** Post-Render ***********/
            {
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

    MeshStorage& Engine::GetMeshStorage()
    {
        IG_CHECK(instance != nullptr);
        return *instance->meshStorage;
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
