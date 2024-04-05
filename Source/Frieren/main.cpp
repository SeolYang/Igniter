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
        [[maybe_unused]] AssetManager& assetManager = engine.GetAssetManager();

        /* #sy_test Asset System & Mechanism Test */

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
        // const auto homura = registry.create();
        //{
        //     registry.emplace<NameComponent>(homura, "Homura"_fs);

        //    auto& transformComponent = registry.emplace<TransformComponent>(homura);
        //    transformComponent.Position = Vector3{ 10.5f, -30.f, 10.f };
        //    transformComponent.Scale = Vector3{ 0.35f, 0.35f, 0.35f };

        //    auto& staticMeshComponent = registry.emplace<StaticMeshComponent>(homura);
        //    staticMeshComponent.Mesh = homuraMeshFuture.get();
        //    IG_CHECK(staticMeshComponent.Mesh);
        //}

        Guid guids[] = {
            Guid{ "3c8b4a59-2fd9-4f48-9e37-18093bb56acf" },
            Guid{ "4f745e72-67b2-48ba-a7d9-a69ee7d0ed37" },
            Guid{ "51f5f83c-1c65-4ecf-87d2-ccd78660c0f3" },
            Guid{ "64bc86b2-0960-4e6a-8bfe-b16e96a1a3e3" },
            Guid{ "47654f75-741a-479a-846c-5f2743119496" },
            Guid{ "a10161d2-aca1-491a-9673-3d58a2894f19" },
            Guid{ "ab23d0e4-0b61-4a3d-8511-90a8680869eb" },
            Guid{ "bf77c2fa-f940-40c4-9a3b-9b205953a0fc" },
            Guid{ "da784c45-debc-48e1-9619-8c0d4125c348" },
            Guid{ "eaefbad6-fcb4-4e38-9ab2-c5370f2c7746" },
            Guid{ "f9b7cce6-f8e0-40a8-b821-6a6666432625" },
            Guid{ "f891ae20-efad-4b55-92f1-2579de1c6460" },
            Guid{ "fdac596c-938a-4275-8b1b-7d77ae377e84" }
        };

        std::vector<std::future<CachedAsset<StaticMesh>>> staticMeshFutures{};
        for (Guid guid : guids)
        {
            staticMeshFutures.emplace_back(std::async(
                std::launch::async,
                [&assetManager](const Guid guid)
                {
                    return assetManager.LoadStaticMesh(guid);
                }, guid));
        }

        for (std::future<CachedAsset<StaticMesh>>& staticMeshFuture : staticMeshFutures)
        {
            const auto newEntity{ registry.create() };
            auto& staticMeshComponent{ registry.emplace<StaticMeshComponent>(newEntity) };
            staticMeshComponent.Mesh = staticMeshFuture.get();

            registry.emplace<TransformComponent>(newEntity);
            registry.emplace<NameComponent>(newEntity, staticMeshComponent.Mesh->GetSnapshot().Info.GetVirtualPath());
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
