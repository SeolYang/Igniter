#include "Frieren/Frieren.h"
#include "Igniter/Core/String.h"
#include "Igniter/Core/FrameManager.h"
#include "Igniter/Input/InputManager.h"
#include "Igniter/Asset/AssetManager.h"
#include "Igniter/Gameplay/World.h"
#include "Igniter/ImGui/ImGuiCanvas.h"
#include "Igniter/ImGui/ImGuiRenderer.h"
#include "Igniter/Component/StaticMeshComponent.h"
#include "Igniter/Component/NameComponent.h"
#include "Frieren/Game/System/TestGameSystem.h"
#include "Frieren/Game/Component/FpsCameraArchetype.h"
#include "Frieren/Game/Component/RandMovementComponent.h"
#include "Frieren/Gui/EditorCanvas.h"
#include "Frieren/Application/TestApp.h"

namespace fe
{
    TestApp::TestApp(const ig::AppDesc& desc) :
        Application(desc)
    {
        /* #sy_test 입력 매니저 테스트 */
        ig::InputManager& inputManager = ig::Engine::GetInputManager();
        inputManager.MapAction("MoveLeft"_fs, ig::EInput::A);
        inputManager.MapAction("MoveRight"_fs, ig::EInput::D);
        inputManager.MapAction("MoveForward"_fs, ig::EInput::W);
        inputManager.MapAction("MoveBackward"_fs, ig::EInput::S);
        inputManager.MapAction("MoveUp"_fs, ig::EInput::MouseRB);
        inputManager.MapAction("MoveDown"_fs, ig::EInput::MouseLB);
        inputManager.MapAction("Sprint"_fs, ig::EInput::Shift);

        inputManager.MapAxis("TurnYaw"_fs, ig::EInput::MouseDeltaX, 1.f);
        inputManager.MapAxis("TurnAxis"_fs, ig::EInput::MouseDeltaY, 1.f);

        inputManager.MapAction(ig::String("TogglePossessCamera"), ig::EInput::Space);
        /********************************/

        ///* #sy_test 에셋 시스템 테스트 + 게임 플레이 프레임워크 테스트 */
        /*
        [[maybe_unused]] AssetManager& assetManager = Igniter::GetAssetManager();
         Registry& registry = world->GetRegistry();
         Guid homuraMeshGuids[] = {
            Guid{"0c5a8c40-a6d9-4b6a-bcc9-616b3cef8450"},
            Guid{"5e6a8a5a-4358-47a9-93e6-0e7ed165dbc0"},
            Guid{"ad4921ad-3654-4ce4-a626-6568ce399dba"},
            Guid{"9bbce66e-2292-489f-8e6b-c0f9427fff29"},
            Guid{"731d273c-c9e1-4ee1-acaa-6be8d4d54853"},
            Guid{"de7ff053-c224-4623-bb0f-17372c2ddb82"},
            Guid{"f9ed3d69-d906-4b03-9def-16b6b37211e7"},
        };

         std::vector<std::future<ManagedAsset<StaticMesh>>> homuraMeshFutures{};
         for (Guid guid : homuraMeshGuids)
        {
             homuraMeshFutures.emplace_back(std::async(
                 std::launch::async, [&assetManager](const Guid guid) { return assetManager.Load<StaticMesh>(guid); }, guid));
         }

         for (std::future<ManagedAsset<StaticMesh>>& staticMeshFuture : homuraMeshFutures)
        {
             ManagedAsset<StaticMesh> staticMesh{staticMeshFuture.get()};
             if (staticMesh)
             {
                 const auto newEntity{registry.create()};
                 auto& staticMeshComponent{registry.emplace<StaticMeshComponent>(newEntity)};
                 staticMeshComponent.Mesh = staticMesh;

                registry.emplace<TransformComponent>(newEntity);

                registry.emplace<NameComponent>(newEntity, assetManager.Lookup(staticMeshComponent.Mesh)->GetSnapshot().Info.GetVirtualPath());
            }
        }

         std::future<ManagedAsset<StaticMesh>> homuraAxeMeshFutures{
             std::async(std::launch::async, [&assetManager]() { return assetManager.LoadStaticMesh(Guid{"b77399f0-98ec-47cb-ad80-e7287d5833c2"});
             })};
        {
             ManagedAsset<StaticMesh> staticMesh{homuraAxeMeshFutures.get()};
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

         std::vector<std::future<ManagedAsset<StaticMesh>>> littleTokyoMeshFutures{};
         for (Guid guid : littleTokyoMeshGuids)
        {
             littleTokyoMeshFutures.emplace_back(std::async(
                 std::launch::async, [&assetManager](const Guid guid) { return assetManager.LoadStaticMesh(guid); }, guid));
         }

         for (std::future<ManagedAsset<StaticMesh>>& staticMeshFuture : littleTokyoMeshFutures)
        {
             ManagedAsset<StaticMesh> staticMesh{staticMeshFuture.get()};
             if (staticMesh)
             {
                 const auto newEntity{registry.create()};
                 auto& staticMeshComponent{registry.emplace<StaticMeshComponent>(newEntity)};
                 staticMeshComponent.Mesh = staticMesh;

                registry.emplace<TransformComponent>(newEntity);
                registry.emplace<NameComponent>(newEntity, assetManager.Lookup(staticMeshComponent.Mesh)->GetSnapshot().Info.GetVirtualPath());
            }
        }

         const Entity cameraEntity = CameraArchetype::Create(registry);
        {
             registry.emplace<NameComponent>(cameraEntity, "Camera"_fs);

            TransformComponent& cameraTransform = registry.get<TransformComponent>(cameraEntity);
            cameraTransform.Position = Vector3{0.f, 0.f, -30.f};

            CameraComponent& cameraComponent = registry.get<CameraComponent>(cameraEntity);
            cameraComponent.CameraViewport = Igniter::GetWindow().GetViewport();

            registry.emplace<FpsCameraController>(cameraEntity);
        }
        */
        /********************************************/

        /* #sy_test ImGui 통합 테스트 */
        editorCanvas = MakePtr<EditorCanvas>(*this);
        /************************************/

        /* #sy_test Game Mode 타입 메타 정보 테스트 */
        gameSystem = std::move(*entt::resolve<TestGameSystem>().func(ig::meta::GameSystemConstructFunc).invoke({}).try_cast<ig::Ptr<ig::GameSystem>>());
        /************************************/

        // Json dumpedWorld{};
        //[[maybe_unused]] std::string worldDumpTest = world->Serialize(dumpedWorld).dump();
        // Igniter::GetAssetManager().Import("TestMap"_fs, {.WorldToSerialize = world.get()});
        // world = MakePtr<World>();
        // world->Deserialize(dumpedWorld);

        ig::AssetManager& assetManager = ig::Engine::GetAssetManager();
        ig::World& worldInstance = ig::Engine::GetWorld();
        ig::Registry& registry = worldInstance.GetRegistry();
        FpsCmaeraArchetype::Create(registry, true);

        for (ig::U32 axeGridX = 0; axeGridX < kAxeGridSizeX; ++axeGridX)
        {
            for (ig::U32 axeGridY = 0; axeGridY < kAxeGridSizeY; ++axeGridY)
            {
                for (ig::U32 axeGridZ = 0; axeGridZ < kAxeGridSizeZ; ++axeGridZ)
                {
                    ig::Entity newAxeEentity = registry.create();
                    ig::TransformComponent& transform = registry.emplace<ig::TransformComponent>(newAxeEentity);
                    transform.Position = kAxeOffset + (kAxeSpaceInterval * ig::Vector3{(ig::F32)axeGridX, (ig::F32)axeGridY, (ig::F32)axeGridZ});
                    ig::StaticMeshComponent& staticMeshComponent = registry.emplace<ig::StaticMeshComponent>(newAxeEentity);
                    staticMeshComponent.Mesh = assetManager.Load<ig::StaticMesh>("Axe_Axe_0"_fs);
                    ig::NameComponent& nameComponent = registry.emplace<ig::NameComponent>(newAxeEentity);
                    nameComponent.Name = ig::String(std::format("Axe ({}, {}, {})", axeGridX, axeGridY, axeGridZ));
                    RandMovementComponent& randComp = registry.emplace<RandMovementComponent>(newAxeEentity);
                    randComp.MoveDirection = ig::Vector3{
                        ig::Random(-1.f, 1.f),
                        ig::Random(-1.f, 1.f),
                        ig::Random(-1.f, 1.f)};
                    randComp.MoveDirection.Normalize();
                    randComp.MoveSpeed = ig::Random(0.f, 5.f);

                    randComp.Rotation = ig::Vector3{ig::Random(-1.f, 1.f), ig::Random(-1.f, 1.f), ig::Random(-1.f, 1.f)};
                    randComp.RotateSpeed = ig::Random(0.f, 15.f);
                }
            }
        }

        // worldInstance = ig::World{assetManager, assetManager.Load<ig::Map>(ig::Guid{"92d1aad6-7d75-41a4-be10-c9f8bfdb787e"})};

        ig::ImGuiRenderer& imGuiRenderer = ig::Engine::GetImGuiRenderer();
        imGuiRenderer.SetTargetCanvas(editorCanvas.get());
    }

    TestApp::~TestApp()
    {
        // Empty
    }

    void TestApp::Update(const float deltaTime)
    {
        ZoneScoped;

        ig::World& world = ig::Engine::GetWorld();
        gameSystem->Update(deltaTime, world);

        // #sy_test Multithreaded component update
        ig::Registry& registry = world.GetRegistry();
        auto view = registry.view<ig::TransformComponent, const RandMovementComponent>();
        tf::Executor& taskExecutor = ig::Engine::GetTaskExecutor();
        tf::Taskflow rootTaskFlow;
        rootTaskFlow.for_each(
            view.begin(), view.end(),
            [&view, deltaTime](const ig::Entity entity)
            {
                ig::TransformComponent& transform = view.get<ig::TransformComponent>(entity);
                const RandMovementComponent& randMove = view.get<const RandMovementComponent>(entity);
                transform.Position += (deltaTime * randMove.MoveDirection * randMove.MoveSpeed);
                transform.Rotation *= ig::Quaternion::CreateFromYawPitchRoll(deltaTime * randMove.Rotation * randMove.RotateSpeed);
            });
        taskExecutor.run(rootTaskFlow).wait();
    }

    void TestApp::SetGameSystem(ig::Ptr<ig::GameSystem> newGameSystem)
    {
        gameSystem = std::move(newGameSystem);
    }
} // namespace fe
