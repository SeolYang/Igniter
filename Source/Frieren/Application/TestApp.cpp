#include "Frieren/Frieren.h"
#include "Igniter/Core/String.h"
#include "Igniter/Core/FrameManager.h"
#include "Igniter/Input/InputManager.h"
#include "Igniter/Asset/AssetManager.h"
#include "Igniter/Core/BoundingVolume.h"
#include "Igniter/Gameplay/World.h"
#include "Igniter/ImGui/ImGuiCanvas.h"
#include "Igniter/Component/StaticMeshComponent.h"
#include "Igniter/Component/NameComponent.h"
#include "Igniter/Component/MaterialComponent.h"
#include "Igniter/Component/LightArchetype.h"
#include "Igniter/Component/StaticMeshArchetype.h"
#include "Frieren/Game/System/TestGameSystem.h"
#include "Frieren/Game/Component/FpsCameraArchetype.h"
#include "Frieren/Game/Component/RandMovementComponent.h"
#include "Frieren/Gui/EditorCanvas.h"
#include "Frieren/Application/TestApp.h"

namespace fe
{
    TestApp::TestApp(const ig::AppDesc& desc)
        : Application(desc)
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

        /* #sy_test ImGui 통합 테스트 */
        editorCanvas = MakePtr<EditorCanvas>(*this);
        /************************************/

        /* #sy_test Game Mode 타입 메타 정보 테스트 */
        gameSystem = std::move(*entt::resolve<TestGameSystem>().func(ig::meta::GameSystemConstructFunc).invoke({}).try_cast<ig::Ptr<ig::GameSystem>>());
        /************************************/

        ig::World& worldInstance = ig::Engine::GetWorld();
        ig::Registry& registry = worldInstance.GetRegistry();
        const ig::Entity mainCam = FpsCameraArchetype::Create(&registry);
        auto& cameraComponent = registry.get<ig::CameraComponent>(mainCam);
        cameraComponent.bIsMainCamera = true;

        ig::AssetManager& assetManager = ig::Engine::GetAssetManager();
        ig::ManagedAsset<ig::StaticMesh> axeStaticMesh = assetManager.Load<ig::StaticMesh>("sphere_Cube.001_0"_fs);
        IG_VERIFY(assetManager.Clone(axeStaticMesh, (kAxeGridSizeX * kAxeGridSizeY * kAxeGridSizeZ) - 1));
        ig::ManagedAsset<ig::Material> axeMaterial = assetManager.Load<ig::Material>("Axe"_fs);
        IG_VERIFY(assetManager.Clone(axeMaterial, (kAxeGridSizeX * kAxeGridSizeY * kAxeGridSizeZ) - 1));

        for (ig::U32 axeGridX = 0; axeGridX < kAxeGridSizeX; ++axeGridX)
        {
            for (ig::U32 axeGridY = 0; axeGridY < kAxeGridSizeY; ++axeGridY)
            {
                for (ig::U32 axeGridZ = 0; axeGridZ < kAxeGridSizeZ; ++axeGridZ)
                {
                    ig::Entity newAxeEntity = registry.create();
                    ig::TransformComponent& transform = registry.emplace<ig::TransformComponent>(newAxeEntity);
                    transform.Position = kAxeOffset + (kAxeSpaceInterval * ig::Vector3{(ig::F32)axeGridX, (ig::F32)axeGridY, (ig::F32)axeGridZ});
                    ig::StaticMeshComponent& staticMeshComponent = registry.emplace<ig::StaticMeshComponent>(newAxeEntity);
                    staticMeshComponent.Mesh = axeStaticMesh;
                    registry.emplace<ig::MaterialComponent>(newAxeEntity, axeMaterial);
                    ig::NameComponent& nameComponent = registry.emplace<ig::NameComponent>(newAxeEntity);
                    nameComponent.Name = ig::String(std::format("Axe ({}, {}, {})", axeGridX, axeGridY, axeGridZ));
                    auto& lightComponent = registry.emplace<ig::LightComponent>(newAxeEntity);
                    lightComponent.Property.FalloffRadius = 2.f;

                    RandMovementComponent& randComp = registry.emplace<RandMovementComponent>(newAxeEntity);
                    randComp.MoveDirection = ig::Vector3{
                        ig::Random(-1.f, 1.f),
                        ig::Random(-1.f, 1.f),
                        ig::Random(-1.f, 1.f)};
                    randComp.MoveDirection.Normalize();
                    randComp.MoveSpeed = ig::Random(0.1f, 1.5f);

                    //randComp.Rotation = ig::Vector3{ig::Random(-1.f, 1.f), ig::Random(-1.f, 1.f), ig::Random(-1.f, 1.f)};
                    //randComp.RotateSpeed = ig::Random(0.f, 15.f);
                }
            }
        }

        // constexpr ig::Size kNumLights = 12;
        // ig::AssetManager& assetManager = ig::Engine::GetAssetManager();
        // ig::ManagedAsset<ig::StaticMesh> axeStaticMesh = assetManager.Load<ig::StaticMesh>("Axe_Axe_0"_fs);
        // assetManager.Clone(axeStaticMesh, (ig::U32)kNumLights - 1);

        // constexpr ig::F32 kLightRadius = 5.f;
        // constexpr ig::F32 kLightDistance = 100.f;
        // ig::F32 lightZ = cameraComponent.NearZ - kLightRadius;
        // for (ig::Index lightIdx = 0; lightIdx < kNumLights; ++lightIdx)
        //{
        //     const ig::Entity lightEntity = ig::PointLightArchetype::Create(&registry);
        //     auto& lightComponent = registry.get<ig::LightComponent>(lightEntity);
        //     lightComponent.Property.FalloffRadius = kLightRadius;
        //     auto& transformComponent = registry.get<ig::TransformComponent>(lightEntity);
        //     transformComponent.Position.z = lightZ;
        //     transformComponent.Scale = ig::Vector3{4.f, 4.f, 4.f};
        //     lightZ += kLightDistance;
        //     auto& staticMeshComponent = registry.emplace<ig::StaticMeshComponent>(lightEntity);
        //     staticMeshComponent.Mesh = axeStaticMesh;
        //     registry.emplace<ig::MaterialComponent>(lightEntity);
        // }
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

    void TestApp::OnImGui()
    {
        editorCanvas->OnImGui();
    }

    void TestApp::SetGameSystem(ig::Ptr<ig::GameSystem> newGameSystem)
    {
        gameSystem = std::move(newGameSystem);
    }
} // namespace fe
