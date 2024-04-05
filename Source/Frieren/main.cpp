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

        Guid staticMeshGuids[] = {
            Guid{ "0c5a8c40-a6d9-4b6a-bcc9-616b3cef8450" },
            Guid{ "5e6a8a5a-4358-47a9-93e6-0e7ed165dbc0" },
            Guid{ "ad4921ad-3654-4ce4-a626-6568ce399dba" },
            Guid{ "9bbce66e-2292-489f-8e6b-c0f9427fff29" },
            Guid{ "731d273c-c9e1-4ee1-acaa-6be8d4d54853" },
            Guid{ "de7ff053-c224-4623-bb0f-17372c2ddb82" },
            Guid{ "f9ed3d69-d906-4b03-9def-16b6b37211e7" },
        };

        std::vector<std::future<CachedAsset<StaticMesh>>> staticMeshFutures{};
        for (Guid guid : staticMeshGuids)
        {
            staticMeshFutures.emplace_back(std::async(
                std::launch::async,
                [&assetManager](const Guid guid)
                {
                    return assetManager.LoadStaticMesh(guid);
                },
                guid));
        }

        for (std::future<CachedAsset<StaticMesh>>& staticMeshFuture : staticMeshFutures)
        {
            CachedAsset<StaticMesh> staticMesh{ staticMeshFuture.get() };
            if (staticMesh)
            {
                const auto newEntity{ registry.create() };
                auto& staticMeshComponent{ registry.emplace<StaticMeshComponent>(newEntity) };
                staticMeshComponent.Mesh = std::move(staticMesh);

                registry.emplace<TransformComponent>(newEntity);
                registry.emplace<NameComponent>(newEntity, staticMeshComponent.Mesh->GetSnapshot().Info.GetVirtualPath());
            }
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
