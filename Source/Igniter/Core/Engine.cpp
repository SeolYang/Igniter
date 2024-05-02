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
#include "Igniter/ImGui/ImGuiRenderer.h"
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
        imguiRenderer = MakePtr<ImGuiRenderer>(*window, *renderContext);
        ////////////////////////////////////////////////////

        bInitialized = true;
    }

    Igniter::~Igniter()
    {
        IG_LOG(Engine, Info, "Extinguishing Engine Runtime.");

        //////////////////////// L2 ////////////////////////
        imguiRenderer.reset();
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
            }

            /************* Render *************/
            {
                // #sy_todo 렌더러 인터페이스 및 렌더러 독립 (어플리케이션 단으로 이동/스왑 체인은 RenderContext로 이동)
                application.Render(localFrameIdx);
                // #sy_note 어플리케이션 렌더러-ImGui 렌더러 간의 큐 동기화는?
                // main gfx에서 하나 만들어서 수동으로 동기화?
                // swapchain에 가장 마지막으로 접근하는 queue가 어떤 것 인지 알 수 있어야 한다..
                // 좀더 정확하게 알기 위해서, D3D12의 명령어 Submisison에 대해 복기하기 <<
                // 기억상으론, Cmd Queue에 제출되는 순서대로 명령어들이 Serialize 되고, Submit 간에 일종의 베리어가 생겨서
                // 같은 큐 상에서의 서로 다른 Submit 간의 명령어들 실행 순서가 보존된다고 기억함
                // 또 스왑 체인 렌더 타겟의 최종 상태 정보도 알아야하는데..
                // 결국 텍스처 자원에 대한 상태(즉 텍스처 레이아웃)를 추적해야 할지도 모르겠다
                // 버퍼는 뭐.. Stateless 니까 상관 없을 것 같다
                // 아니면 간단하게, Swapchain은 어떤 렌더러 스코프에서 사용 할 때, 최종적으로 Present 상태(또는 Common)에
                // 있도록 룰을 정하는 것도 나쁘진 않을 것 같기도 하다.
                // 아니면 애초에 항상 Common 상태를 유지 하던가.
                // 결국 최종적으로 Swapchian에 대한 연산을 어떤 큐가 하는지는 알아내야 할 수도 있다.
                // 예시. Compute Queue 또는 AsyncCopy 에서 Swapchain에 결과를 쓰는 경우
                // 반대로 성능을 조금 포기하고, 단순히 Quad를 통해 Swapchain의 결과를 Main Gfx Queue를 통해 쓰도록 강제도 할 수 있다
                // https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html#d3d12_barrier_layout_present
                // Calling **ExecuteCommandLists** twice in succession (from the same thread, or different threads)
                // guarantees that the first workload (A) finishes before the second workload (B).
                // 가장 마지막 제안을 수용하는게 그나마 현실적이면서 가장 간단한 방법인 것 같긴 하다..
                // 아니면 렌더 그래프를 만들던가! -> 웬만하면 피하고 싶음!
                imguiRenderer->Render(localFrameIdx, imguiCanvas);
            }

            /*********** Post-Render ***********/
            {
                localFrameSyncs[localFrameIdx] = application.PostRender(localFrameIdx);
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
