#include <Frieren.h>
#include <Asset/StaticMesh.h>
#include <Asset/Texture.h>
#include <Asset/Material.h>
#include <Asset/AssetManager.h>
#include <Component/CameraArchetype.h>
#include <Component/CameraComponent.h>
#include <Component/NameComponent.h>
#include <Component/StaticMeshComponent.h>
#include <Component/TransformComponent.h>
#include <Core/Event.h>
#include <Core/Handle.h>
#include <Core/Engine.h>
#include <Core/InputManager.h>
#include <Core/Window.h>
#include <D3D12/CommandQueue.h>
#include <D3D12/GPUBuffer.h>
#include <D3D12/GPUBufferDesc.h>
#include <D3D12/GpuTexture.h>
#include <D3D12/GPUView.h>
#include <D3D12/RenderDevice.h>
#include <Gameplay/ComponentRegistry.h>
#include <Gameplay/GameInstance.h>
#include <ImGui/CachedStringDebugger.h>
#include <ImGui/EntityInsepctor.h>
#include <ImGui/EntityList.h>
#include <ImGui/ImGuiCanvas.h>
#include <ImGui/ImGuiLayer.h>
#include <ImGui/StatisticsPanel.h>
#include <Render/GPUViewManager.h>
#include <Render/GpuUploader.h>
#include <Core/String.h>
#include <MainLayer.h>
#include <FpsCameraController.h>
#include <TestGameMode.h>

int main()
{
    using namespace ig;
    using namespace fe;
    int result = 0;
    {
        Igniter engine;
        Window& window = engine.GetWindow();
        InputManager& inputManager = engine.GetInputManager();
        GameInstance& gameInstance = engine.GetGameInstance();
        AssetManager& assetManager = engine.GetAssetManager();

        /* #sy_test Asset System & Mechanism Test */
        std::future<CachedAsset<StaticMesh>> ashMeshFuture = std::async(
            std::launch::async,
            [&assetManager]()
            {
                return assetManager.LoadStaticMesh(xg::Guid{ "e42f93b4-e57d-44ae-9fde-302013c6e9e8" });
            });

        std::future<CachedAsset<Texture>> ashBodyTexFuture = std::async(
            std::launch::async,
            [&assetManager]()
            {
                return assetManager.LoadTexture(xg::Guid{ "4b2b2556-7d81-4884-ba50-777392ebc9ee" });
            });

        std::future<CachedAsset<StaticMesh>> homuraMeshFuture = std::async(
            std::launch::async,
            [&assetManager]()
            {
                return assetManager.LoadStaticMesh(xg::Guid{ "a717ff6e-129b-4c80-927a-a786a0b21128" });
            });

        std::future<CachedAsset<Texture>> homuraBodyTexFuture = std::async(
            std::launch::async,
            [&assetManager]()
            {
                return assetManager.LoadTexture(xg::Guid{ "87949751-3431-45c7-bd57-0a1518649511" });
            });

        std::future<xg::Guid> ashBodyMatCreateFuture{
            std::async(std::launch::async,
                       [&assetManager, &ashBodyTexFuture]
                       {
                           // return assetManager.CreateMaterial(MaterialCreateDesc{ .VirtualPath = "Ash\\Body"_fs, .Diffuse = ashBodyTexFuture.get() });
                           return assetManager.CreateMaterial(MaterialCreateDesc{ .VirtualPath = "Ash\\Body"_fs, .Diffuse = assetManager.LoadTexture("Engine\\Default"_fs) });
                       })
        };

        std::future<xg::Guid> homuraBodyMatCreateFuture{
            std::async(std::launch::async,
                       [&assetManager, &homuraBodyTexFuture]
                       {
                           return assetManager.CreateMaterial(MaterialCreateDesc{ .VirtualPath = "Homura\\Body"_fs, .Diffuse = homuraBodyTexFuture.get() });
                       })
        };

        std::future<CachedAsset<Material>> ashBodyMatFuture{
            std::async(std::launch::async,
                       [&assetManager, &ashBodyMatCreateFuture]()
                       {
                           return assetManager.LoadMaterial(ashBodyMatCreateFuture.get());
                       })
        };

        std::future<CachedAsset<Material>> homuraBodyMatFuture{
            std::async(std::launch::async,
                       [&assetManager, &homuraBodyMatCreateFuture]()
                       {
                           return assetManager.LoadMaterial(homuraBodyMatCreateFuture.get());
                       })
        };
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

        inputManager.BindAction(String("TogglePossessCamera"), EInput::Space);
        /********************************/

        /* #sy_test ECS based Game flow & logic tests */
        Registry& registry = gameInstance.GetRegistry();
        gameInstance.SetGameMode(std::make_unique<TestGameMode>());
        const auto homura = registry.create();
        {
            registry.emplace<NameComponent>(homura, "Homura"_fs);

            auto& transformComponent = registry.emplace<TransformComponent>(homura);
            transformComponent.Position = Vector3{ 10.5f, -30.f, 10.f };
            transformComponent.Scale = Vector3{ 0.35f, 0.35f, 0.35f };

            auto& staticMeshComponent = registry.emplace<StaticMeshComponent>(homura);
            staticMeshComponent.Mesh = homuraMeshFuture.get();
            staticMeshComponent.Mat = homuraBodyMatFuture.get();
            IG_CHECK(staticMeshComponent.Mesh);
            IG_CHECK(staticMeshComponent.Mat);
        }

        const auto ash = registry.create();
        {
            registry.emplace<NameComponent>(ash, "Ash"_fs);

            auto& transformComponent = registry.emplace<TransformComponent>(ash);
            transformComponent.Position = Vector3{ -10.f, -30.f, 35.f };
            transformComponent.Scale = Vector3{ 40.f, 40.f, 40.f };

            auto& staticMeshComponent = registry.emplace<StaticMeshComponent>(ash);
            staticMeshComponent.Mesh = ashMeshFuture.get();
            staticMeshComponent.Mat = ashBodyMatFuture.get();
            IG_CHECK(staticMeshComponent.Mesh);
            IG_CHECK(staticMeshComponent.Mat);
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
