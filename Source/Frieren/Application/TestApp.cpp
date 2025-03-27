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
#include "Igniter/Audio/AudioSystem.h"
#include "Igniter/Audio/AudioSourceComponent.h"
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
         auto& cameraTransform = registry.get<ig::TransformComponent>(mainCam);
         cameraTransform.Position.z = 0.f;
         auto& camController = registry.get<FpsCameraController>(mainCam);
         camController.MovementPower = 1.f;
         
         ig::AssetManager& assetManager = ig::Engine::GetAssetManager();
         //testStaticMesh = assetManager.Load<ig::StaticMesh>("bunny_defaultobject_0"_fs);
         testStaticMesh = assetManager.Load<ig::StaticMesh>("sphere_Cube.001_0"_fs);
         //testStaticMesh = assetManager.Load<ig::StaticMesh>("DragonAttenuation_Dragon_1"_fs);
         testMaterial = assetManager.Load<ig::Material>(ig::Guid{ig::DefaultMaterialGuid});

#ifdef TEST_ENTITY
        ig::Entity newEntity = ig::StaticMeshArchetype::Create(&registry);
        ig::TransformComponent& transformComponent = registry.get<ig::TransformComponent>(newEntity);
        transformComponent.Position.z = 0.f;
        transformComponent.Scale = ig::Vector3{1.f, 1.f, 1.f};
        ig::StaticMeshComponent& meshComponent = registry.get<ig::StaticMeshComponent>(newEntity);
        meshComponent.Mesh = testStaticMesh;
        ig::MaterialComponent& materialComponent = registry.get<ig::MaterialComponent>(newEntity);
        materialComponent.Instance = testMaterial;
#endif

#ifdef TEST_LIGHT
        ig::Entity newLight = ig::PointLightArchetype::Create(&registry);
        ig::TransformComponent& testLightTransform = registry.get<ig::TransformComponent>(newLight);
        testLightTransform.Position.z = -650.f;
        ig::LightComponent& testLightComponent = registry.get<ig::LightComponent>(newLight);
        testLightComponent.Property.Intensity = 1000.f;
        testLightComponent.Property.FalloffRadius = 500.f;
#endif

#if !defined(TEST_ENTITY) && !defined(TEST_LIGHT)
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
                    staticMeshComponent.Mesh = testStaticMesh;
                    ig::NameComponent& nameComponent = registry.emplace<ig::NameComponent>(newAxeEntity);
                    nameComponent.Name = ig::String(std::format("Axe ({}, {}, {})", axeGridX, axeGridY, axeGridZ));
                    ig::MaterialComponent& matComp = registry.get<ig::MaterialComponent>(newAxeEntity);
                    matComp.Instance = testMaterial;
        
                    RandMovementComponent& randComp = registry.emplace<RandMovementComponent>(newAxeEntity);
                    randComp.Rotation = ig::Vector3{ig::Random(-1.f, 1.f), ig::Random(-1.f, 1.f), ig::Random(-1.f, 1.f)};
                    randComp.RotateSpeed = ig::Random(0.f, 15.f);
                }
            }
        }
#endif
        
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
                    lightComponent.Property.FalloffRadius = 15.f;
                    lightComponent.Property.Intensity = 20.f;
                    lightComponent.Property.Color = ig::Vector3{ig::Random(0.f, 1.f), ig::Random(0.f, 1.f), ig::Random(0.f, 1.f)};

                    RandMovementComponent& randComp = registry.emplace<RandMovementComponent>(newLightEntity);
                    randComp.MoveDirection = ig::Vector3{
                        ig::Random(-1.f, 1.f),
                        ig::Random(-1.f, 1.f),
                        ig::Random(-1.f, 1.f)
                    };
                    randComp.MoveDirection.Normalize();
                    randComp.MoveSpeed = ig::Random(1.f, 5.5f);
                }
            }
        }

        testAudioClip = ig::Engine::GetAudioSystem().CreateClip("Resources/BGM/battle_cry_Ver2.0_Loop.ogg");
        const ig::Entity audioSourceEntity = registry.create();
        ig::AudioSourceComponent& audioSource = registry.emplace<ig::AudioSourceComponent>(audioSourceEntity);
        audioSource.Clip = testAudioClip;
        audioSource.NextEvent = ig::EAudioEvent::Play;
        audioSource.bLoop = true;
        audioSource.bShouldUpdatePropertiesOnThisFrame = true;
    }

    TestApp::~TestApp()
    {
        ig::AssetManager& assetManager = ig::Engine::GetAssetManager();
        assetManager.Unload(testStaticMesh);
        assetManager.Unload(testMaterial);

        ig::Engine::GetAudioSystem().Destroy(testAudioClip);
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
