#include <Frieren.h>
#include <Core/Handle.h>
#include <Core/Engine.h>
#include <Core/Window.h>
#include <Core/String.h>
#include <Asset/StaticMesh.h>
#include <Asset/AssetManager.h>
#include <Component/CameraArchetype.h>
#include <Component/CameraComponent.h>
#include <Component/NameComponent.h>
#include <Component/StaticMeshComponent.h>
#include <Component/TransformComponent.h>
#include <ImGui/ImGuiCanvas.h>
#include <ImGui/ImGuiLayer.h>
#include <ImGui/MainLayer.h>
#include <Input/InputManager.h>
#include <Gameplay/GameInstance.h>
#include <Game/Component/FpsCameraController.h>
#include <Game/Mode/TestGameMode.h>

int main()
{
    using namespace ig;
    using namespace fe;
    int result = 0;
    Igniter::Ignite();
    {
        Window& window = Igniter::GetWindow();
        InputManager& inputManager = Igniter::GetInputManager();
        GameInstance& gameInstance = Igniter::GetGameInstance();
        [[maybe_unused]] AssetManager& assetManager = Igniter::GetAssetManager();

        /* #sy_test Input Manager Test */
        inputManager.MapAction("MoveLeft"_fs, EInput::A);
        inputManager.MapAction("MoveRight"_fs, EInput::D);
        inputManager.MapAction("MoveForward"_fs, EInput::W);
        inputManager.MapAction("MoveBackward"_fs, EInput::S);
        inputManager.MapAction("MoveUp"_fs, EInput::MouseRB);
        inputManager.MapAction("MoveDown"_fs, EInput::MouseLB);
        inputManager.MapAction("Sprint"_fs, EInput::Shift);

        inputManager.MapAxis("TurnYaw"_fs, EInput::MouseDeltaX, 1.f);
        inputManager.MapAxis("TurnAxis"_fs, EInput::MouseDeltaY, 1.f);

        inputManager.MapAction(String("TogglePossessCamera"), EInput::Space);
        /********************************/

        /* #sy_test ECS based Game flow & logic tests */
        Registry& registry = gameInstance.GetRegistry();
        gameInstance.SetGameMode(std::make_unique<TestGameMode>());

        Guid homuraMeshGuids[] = {
            Guid{"0c5a8c40-a6d9-4b6a-bcc9-616b3cef8450"},
            Guid{"5e6a8a5a-4358-47a9-93e6-0e7ed165dbc0"},
            Guid{"ad4921ad-3654-4ce4-a626-6568ce399dba"},
            Guid{"9bbce66e-2292-489f-8e6b-c0f9427fff29"},
            Guid{"731d273c-c9e1-4ee1-acaa-6be8d4d54853"},
            Guid{"de7ff053-c224-4623-bb0f-17372c2ddb82"},
            Guid{"f9ed3d69-d906-4b03-9def-16b6b37211e7"},
        };

        std::vector<std::future<Handle<StaticMesh>>> homuraMeshFutures{};
        for (Guid guid : homuraMeshGuids)
        {
            homuraMeshFutures.emplace_back(std::async(
                std::launch::async, [&assetManager](const Guid guid) { return assetManager.LoadStaticMesh(guid); }, guid));
        }

        for (std::future<Handle<StaticMesh>>& staticMeshFuture : homuraMeshFutures)
        {
            Handle<StaticMesh> staticMesh{staticMeshFuture.get()};
            if (staticMesh)
            {
                const auto newEntity{registry.create()};
                auto& staticMeshComponent{registry.emplace<StaticMeshComponent>(newEntity)};
                staticMeshComponent.Mesh = staticMesh;

                registry.emplace<TransformComponent>(newEntity);

                registry.emplace<NameComponent>(newEntity, assetManager.Lookup(staticMeshComponent.Mesh)->GetSnapshot().Info.GetVirtualPath());
            }
        }

        std::future<Handle<StaticMesh>> homuraAxeMeshFutures{
            std::async(std::launch::async, [&assetManager]() { return assetManager.LoadStaticMesh(Guid{"b77399f0-98ec-47cb-ad80-e7287d5833c2"}); })};
        {
            Handle<StaticMesh> staticMesh{homuraAxeMeshFutures.get()};
            if (staticMesh)
            {
                const auto newEntity{registry.create()};
                auto& staticMeshComponent{registry.emplace<StaticMeshComponent>(newEntity)};
                staticMeshComponent.Mesh = std::move(staticMesh);

                registry.emplace<TransformComponent>(newEntity);
                registry.emplace<NameComponent>(newEntity, assetManager.Lookup(staticMeshComponent.Mesh)->GetSnapshot().Info.GetVirtualPath());
            }
        }

        Guid littleTokyoMeshGuids[] = {
            Guid{"b7cdf76b-12ca-4476-8c83-bcb0ca77bd5a"},
            Guid{"573ad7dc-62af-46b0-8388-690bf473abe3"},
            Guid{"52b2cb76-d1fe-4e5f-97b8-0138391bbea4"},
            Guid{"7204d4a4-8899-468d-ab40-d32011817c63"},
            Guid{"ce3e807e-73a0-44f2-bd68-e5694b438ef7"},
            Guid{"05e0214e-ffc8-4d0b-8021-df4ca53652ba"},
            Guid{"3ab251d9-de45-4de9-a6a0-db193e65d161"},
            Guid{"f1d4855c-19df-4ec7-97ee-54e3cc3fa38b"},
            Guid{"21a63b13-4a92-460b-81bf-064e4950b848"},
            Guid{"60f75ff6-5e72-4e2f-8d4f-71dd94de5105"},
            Guid{"4f1cd7b8-fcd9-4bab-8bf1-0fc1268ccb02"},
            Guid{"3834a543-15cc-471c-8a28-2715201777f7"},
            Guid{"815e2b44-1ede-44ba-a8b5-d20ccc44d7f2"},
            Guid{"43c9dcec-28df-45cc-a881-e154b1bc38d8"},
            Guid{"40a98f09-f5d2-4944-ab41-1ce1da88c888"},
            Guid{"2d2625b8-27d7-43cb-8895-af5164bcf0f9"},
            Guid{"7da574d2-5d54-43fe-be78-b394dcb5a689"},
            Guid{"70f20985-0a11-4b4c-bdc6-30adb116080c"},
            Guid{"657015c8-8d4b-496a-9ba5-54c5a9d6aa70"},
            Guid{"fcb4a3d5-2c54-41bb-9a25-1e8ef20cb7f6"},
            Guid{"cd08f473-47d9-4e53-b998-be27b28d1d6b"},
            Guid{"f8dad50c-7331-42a4-be15-72923762a76a"},
        };

        std::vector<std::future<Handle<StaticMesh>>> littleTokyoMeshFutures{};
        for (Guid guid : littleTokyoMeshGuids)
        {
            littleTokyoMeshFutures.emplace_back(std::async(
                std::launch::async, [&assetManager](const Guid guid) { return assetManager.LoadStaticMesh(guid); }, guid));
        }

        for (std::future<Handle<StaticMesh>>& staticMeshFuture : littleTokyoMeshFutures)
        {
            Handle<StaticMesh> staticMesh{staticMeshFuture.get()};
            if (staticMesh)
            {
                const auto newEntity{registry.create()};
                auto& staticMeshComponent{registry.emplace<StaticMeshComponent>(newEntity)};
                staticMeshComponent.Mesh = staticMesh;

                registry.emplace<TransformComponent>(newEntity);
                registry.emplace<NameComponent>(newEntity, assetManager.Lookup(staticMeshComponent.Mesh)->GetSnapshot().Info.GetVirtualPath());
            }
        }

        /* Camera */
        const Entity cameraEntity = CameraArchetype::Create(registry);
        {
            registry.emplace<NameComponent>(cameraEntity, "Camera"_fs);

            TransformComponent& cameraTransform = registry.get<TransformComponent>(cameraEntity);
            cameraTransform.Position = Vector3{0.f, 0.f, -30.f};

            CameraComponent& cameraComponent = registry.get<CameraComponent>(cameraEntity);
            cameraComponent.CameraViewport = window.GetViewport();

            registry.emplace<FpsCameraController>(cameraEntity);
            registry.emplace<MainCameraTag>(cameraEntity);
        }
        /********************************************/

        /* #sy_test ImGui integration tests */
        ImGuiCanvas& canvas = Igniter::GetImGuiCanvas();
        auto& mainLayer = canvas.AddLayer<MainLayer>(canvas);
        mainLayer.SetVisibility(true);
        /************************************/

        result = Igniter::Execute();
    }
    Igniter::Extinguish();

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

    ~MemoryLeakDetector() { _CrtDumpMemoryLeaks(); }
};

#pragma warning(push)
#pragma warning(disable : 4073)
#pragma init_seg(lib)
MemoryLeakDetector memLeakDetector;
#pragma warning(pop)
#endif