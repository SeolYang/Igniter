#include <Asset/Model.h>
#include <Asset/Texture.h>
#include <Component/CameraArchetype.h>
#include <Component/NameComponent.h>
#include <Component/StaticMeshComponent.h>
#include <Component/TransformComponent.h>
#include <Core/Event.h>
#include <Core/Handle.h>
#include <Core/Igniter.h>
#include <Core/InputManager.h>
#include <Core/Window.h>
#include <D3D12/CommandQueue.h>
#include <D3D12/GPUBuffer.h>
#include <D3D12/GPUBufferDesc.h>
#include <D3D12/GPUView.h>
#include <D3D12/RenderDevice.h>
#include <FpsCameraController.h>
#include <Gameplay/ComponentRegistry.h>
#include <Gameplay/GameInstance.h>
#include <ImGui/CachedStringDebugger.h>
#include <ImGui/EntityInsepctor.h>
#include <ImGui/EntityList.h>
#include <ImGui/ImGuiCanvas.h>
#include <ImGui/ImGuiLayer.h>
#include <MainLayer.h>
#include <ImGui/StatisticsPanel.h>
#include <Render/GPUViewManager.h>
#include <Render/GpuUploader.h>
#include <TestGameMode.h>
#include <future>
#include <iostream>

int main()
{
    using namespace ig;
    using namespace fe;
    int result = 0;
    {
        Igniter engine;
        Window& window = engine.GetWindow();
        InputManager& inputManager = engine.GetInputManager();
        RenderDevice& renderDevice = engine.GetRenderDevice();
        GameInstance& gameInstance = engine.GetGameInstance();
        HandleManager& handleManager = engine.GetHandleManager();
        GpuViewManager& gpuViewManager = engine.GetGPUViewManager();
        GpuUploader& gpuUploader = engine.GetGpuUploader();

        /* #sy_test Asset System & Mechanism Test */
        const xg::Guid ashBodyTexGuid = xg::Guid("4b2b2556-7d81-4884-ba50-777392ebc9ee");
        const xg::Guid ashStaticMeshGuid = xg::Guid("e42f93b4-e57d-44ae-9fde-302013c6e9e8");

        const xg::Guid homuraBodyTexGuid = xg::Guid("87949751-3431-45c7-bd57-0a1518649511");
        const xg::Guid homuraStaticMeshGuid = xg::Guid("a717ff6e-129b-4c80-927a-a786a0b21128");

        std::future<std::optional<Texture>> ashBodyTexFuture = std::async(
            std::launch::async,
            TextureLoader::Load,
            ashBodyTexGuid,
            std::ref(handleManager), std::ref(renderDevice), std::ref(gpuUploader), std::ref(gpuViewManager));

        std::future<std::optional<StaticMesh>> ashStaticMeshFuture = std::async(
            std::launch::async,
            StaticMeshLoader::Load,
            ashStaticMeshGuid,
            std::ref(handleManager), std::ref(renderDevice), std::ref(gpuUploader), std::ref(gpuViewManager));

        std::future<std::optional<StaticMesh>> homuraStaticMeshFuture = std::async(
            std::launch::async,
            StaticMeshLoader::Load,
            homuraStaticMeshGuid,
            std::ref(handleManager), std::ref(renderDevice), std::ref(gpuUploader), std::ref(gpuViewManager));

        std::future<std::optional<Texture>> homuraBodyTexFuture = std::async(
            std::launch::async,
            TextureLoader::Load,
            homuraBodyTexGuid,
            std::ref(handleManager), std::ref(renderDevice), std::ref(gpuUploader), std::ref(gpuViewManager));

        /******************************/

        /* #sy_test Input Manager Test */
        inputManager.BindAction("MoveLeft"_fs, EInput::A);
        inputManager.BindAction("MoveRight"_fs, EInput::D);
        inputManager.BindAction("MoveForward"_fs, EInput::W);
        inputManager.BindAction("MoveBackward"_fs, EInput::S);
        inputManager.BindAction("MoveUp"_fs, EInput::MouseRB);
        inputManager.BindAction("MoveDown"_fs, EInput::MouseLB);
        inputManager.BindAction("Sprint"_fs, EInput::Shift);

        inputManager.BindAxis("TurnYaw"_fs, EInput::MouseDeltaX, 1.f);
        inputManager.BindAxis("TurnAxis"_fs, EInput::MouseDeltaY, 1.f);

        inputManager.BindAction(String("TogglePossessCamera"), EInput::Control);
        /********************************/

        /* #sy_test ECS based Game flow & logic tests */
        Registry& registry = gameInstance.GetRegistry();
        gameInstance.SetGameMode(std::make_unique<TestGameMode>());

        auto homuraBodyTex = homuraBodyTexFuture.get();
        IG_CHECK(homuraBodyTex);
        auto homuraStaticMesh = homuraStaticMeshFuture.get();
        auto homuraStaticMeshHandle = Handle<StaticMesh>{ handleManager, std::move(homuraStaticMesh.value()) };

        const auto homura = registry.create();
        IG_CHECK(homuraStaticMesh);
        {
            registry.emplace<NameComponent>(homura, "Homura"_fs);

            auto& transformComponent = registry.emplace<TransformComponent>(homura);
            transformComponent.Position = Vector3{ 10.5f, -30.f, 10.f };
            transformComponent.Scale = Vector3{ 0.35f, 0.35f, 0.35f };

            auto& staticMeshComponent = registry.emplace<StaticMeshComponent>(homura);
            staticMeshComponent.StaticMeshHandle = homuraStaticMeshHandle.MakeRef();
            staticMeshComponent.DiffuseTex = homuraBodyTex->ShaderResourceView.MakeRef();
            staticMeshComponent.DiffuseTexSampler = homuraBodyTex->TexSampler;
        }

        auto ashBodyTex = ashBodyTexFuture.get();
        IG_CHECK(ashBodyTex);
        auto ashStaticMesh = ashStaticMeshFuture.get();
        IG_CHECK(ashStaticMesh);
        auto ashStaticMeshHandle = Handle<StaticMesh>{ handleManager, std::move(ashStaticMesh.value()) };

        const auto ash = registry.create();
        {
            registry.emplace<NameComponent>(ash, "Ash"_fs);

            auto& transformComponent = registry.emplace<TransformComponent>(ash);
            transformComponent.Position = Vector3{ -10.f, -30.f, 35.f };
            transformComponent.Scale = Vector3{ 40.f, 40.f, 40.f };

            auto& staticMeshComponent = registry.emplace<StaticMeshComponent>(ash);
            staticMeshComponent.StaticMeshHandle = ashStaticMeshHandle.MakeRef();
            staticMeshComponent.DiffuseTex = ashBodyTex->ShaderResourceView.MakeRef();
            staticMeshComponent.DiffuseTexSampler = ashBodyTex->TexSampler;
        }

        /* Camera */
        const Entity cameraEntity = CameraArchetype::Create(registry);
        {
            registry.emplace<NameComponent>(cameraEntity, "Camera"_fs);

            TransformComponent& cameraTransform = registry.get<TransformComponent>(cameraEntity);
            cameraTransform.Position = Vector3{ 0.f, 0.f, -30.f };

            CameraComponent& cameraComponent = registry.get<CameraComponent>(cameraEntity);
            cameraComponent.CameraViewport = window.GetViewport();

            registry.emplace<FpsCameraController>(cameraEntity);
            registry.emplace<MainCameraTag>(cameraEntity);
        }
        /********************************************/

        /* #sy_test ImGui integration tests */
        ImGuiCanvas& canvas = engine.GetImGuiCanvas();
        auto& mainLayer = canvas.AddLayer<MainLayer>(canvas);
        mainLayer.SetVisibility(true);
        /************************************/

        result = engine.Execute();
    }

    return result;
}

#ifdef _CRTDBG_MAP_ALLOC
    #include <crtdbg.h>

struct MemoryLeakDetector
{
    MemoryLeakDetector()
    {
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
        _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    }

    ~MemoryLeakDetector()
    {
        _CrtDumpMemoryLeaks();
    }
};

    #pragma warning(push)
    #pragma warning(disable : 4073)
    #pragma init_seg(lib)
MemoryLeakDetector memLeakDetector;
    #pragma warning(pop)
#endif
