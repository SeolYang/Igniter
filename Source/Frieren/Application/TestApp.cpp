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
#include "Igniter/Component/RenderableTag.h"
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
       //ig::Handle<ig::StaticMesh> bunnySM = assetManager.Load<ig::StaticMesh>("bunny_defaultobject_0"_fs);
        ig::Handle<ig::StaticMesh> bunnySM = assetManager.Load<ig::StaticMesh>("sphere_Cube.001_0"_fs);
        IG_VERIFY(assetManager.Clone(bunnySM, (kAxeGridSizeX * kAxeGridSizeY * kAxeGridSizeZ) - 1));
        ig::Handle<ig::Material> axeMaterial = assetManager.Load<ig::Material>(ig::Guid{ig::DefaultMaterialGuid});
        IG_VERIFY(assetManager.Clone(axeMaterial, (kAxeGridSizeX * kAxeGridSizeY * kAxeGridSizeZ) - 1));
        for (ig::U32 axeGridX = 0; axeGridX < kAxeGridSizeX; ++axeGridX)
        {
            for (ig::U32 axeGridY = 0; axeGridY < kAxeGridSizeY; ++axeGridY)
            {
                for (ig::U32 axeGridZ = 0; axeGridZ < kAxeGridSizeZ; ++axeGridZ)
                {
                    ig::Entity newAxeEntity = ig::StaticMeshArchetype::Create(&registry);
                    ig::TransformComponent& transform = registry.get<ig::TransformComponent>(newAxeEntity);
                    transform.Scale = ig::Vector3{1.f, 1.0f, 1.f};
                    transform.Position = kAxeOffset + (kAxeSpaceInterval * ig::Vector3{(ig::F32)axeGridX, (ig::F32)axeGridY, (ig::F32)axeGridZ});
                    ig::StaticMeshComponent& staticMeshComponent = registry.get<ig::StaticMeshComponent>(newAxeEntity);
                    staticMeshComponent.Mesh = bunnySM;
                    ig::NameComponent& nameComponent = registry.emplace<ig::NameComponent>(newAxeEntity);
                    nameComponent.Name = ig::String(std::format("Axe ({}, {}, {})", axeGridX, axeGridY, axeGridZ));
                    ig::MaterialComponent& matComp = registry.get<ig::MaterialComponent>(newAxeEntity);
                    matComp.Instance = axeMaterial;
        
                    RandMovementComponent& randComp = registry.emplace<RandMovementComponent>(newAxeEntity);
                    // randComp.MoveDirection = ig::Vector3{
                    //     ig::Random(-1.f, 1.f),
                    //     ig::Random(-1.f, 1.f),
                    //     ig::Random(-1.f, 1.f)};
                    // randComp.MoveDirection.Normalize();
                    // randComp.MoveSpeed = ig::Random(0.1f, 1.5f);
        
                    randComp.Rotation = ig::Vector3{ig::Random(-1.f, 1.f), ig::Random(-1.f, 1.f), ig::Random(-1.f, 1.f)};
                    randComp.RotateSpeed = ig::Random(0.f, 15.f);
                }
            }
        }

        for (ig::U32 lightGridX = 0; lightGridX < kLightGridSizeX; ++lightGridX)
        {
            for (ig::U32 lightGridY = 0; lightGridY < kLightGridSizeY; ++lightGridY)
            {
                for (ig::U32 lightGridZ = 0; lightGridZ < kLightGridSizeZ; ++lightGridZ)
                {
                    ig::Entity newLightEntity = registry.create();
                    ig::TransformComponent& transform = registry.emplace<ig::TransformComponent>(newLightEntity);
                    transform.Position = kLightOffset + (kAxeSpaceInterval * ig::Vector3{(ig::F32)lightGridX, (ig::F32)lightGridY, (ig::F32)lightGridZ});
                    ig::NameComponent& nameComponent = registry.emplace<ig::NameComponent>(newLightEntity);
                    nameComponent.Name = ig::String(std::format("Light ({}, {}, {})", lightGridX, lightGridY, lightGridZ));
                    auto& lightComponent = registry.emplace<ig::LightComponent>(newLightEntity);
                    // lightComponent.Property.FalloffRadius = 30.f;
                    lightComponent.Property.FalloffRadius = 15.f;
                    lightComponent.Property.Intensity = 20.f;
                    lightComponent.Property.Color = ig::Vector3{ig::Random(0.f, 1.f), ig::Random(0.f, 1.f), ig::Random(0.f, 1.f)};

                    RandMovementComponent& randComp = registry.emplace<RandMovementComponent>(newLightEntity);
                    randComp.MoveDirection = ig::Vector3{
                        ig::Random(-1.f, 1.f),
                        ig::Random(-1.f, 1.f),
                        ig::Random(-1.f, 1.f)};
                    randComp.MoveDirection.Normalize();
                    randComp.MoveSpeed = ig::Random(1.f, 5.5f);
                }
            }
        }
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
