#include "Frieren/Frieren.h"
#include "Igniter/Component/TransformComponent.h"
#include "Igniter/Component/CameraComponent.h"
#include "Igniter/Input/InputManager.h"
#include "Igniter/Core/Log.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Core/Timer.h"
#include "Igniter/Gameplay/World.h"
#include "Frieren/Game/System/FpsCameraControllSystem.h"
#include "Frieren/Game/Component/FpsCameraController.h"

namespace fe
{
    FpsCameraControllSystem::FpsCameraControllSystem()
    {
        auto& inputManager = ig::Engine::GetInputManager();
        moveLeftActionHandle = inputManager.QueryAction(ig::String("MoveLeft"));
        moveRightActionHandle = inputManager.QueryAction(ig::String("MoveRight"));
        moveForwardActionHandle = inputManager.QueryAction(ig::String("MoveForward"));
        moveBackwardActionHandle = inputManager.QueryAction(ig::String("MoveBackward"));

        moveUpActionHandle = inputManager.QueryAction(ig::String("MoveUp"));
        moveDownActionHandle = inputManager.QueryAction(ig::String("MoveDown"));

        sprintActionHandle = inputManager.QueryAction(ig::String("Sprint"));

        turnYawAxisHandle = inputManager.QueryAxis(ig::String("TurnYaw"));
        turnPitchAxisHandle = inputManager.QueryAxis(ig::String("TurnAxis"));
    }

    void FpsCameraControllSystem::Update(const float deltaTime, ig::World& world)
    {
        ig::Registry& registry = world.GetRegistry();
        const auto& inputManager = ig::Engine::GetInputManager();
        const ig::Action moveLeftAction = inputManager.GetAction(moveLeftActionHandle);
        const ig::Action moveRightAction = inputManager.GetAction(moveRightActionHandle);
        const ig::Action moveForwardAction = inputManager.GetAction(moveForwardActionHandle);
        const ig::Action moveBackwardAction = inputManager.GetAction(moveBackwardActionHandle);
        const ig::Action moveUpAction = inputManager.GetAction(moveUpActionHandle);
        const ig::Action moveDownAction = inputManager.GetAction(moveDownActionHandle);

        const ig::Axis turnYawAxis = inputManager.GetAxis(turnYawAxisHandle);
        const ig::Axis turnPitchAxis = inputManager.GetAxis(turnPitchAxisHandle);

        const ig::Action sprintAction = inputManager.GetAction(sprintActionHandle);

        const auto fpsCamView = registry.view<ig::TransformComponent, FpsCameraController, ig::CameraComponent>();
        for (const ig::Entity entity : fpsCamView)
        {
            auto& transform = fpsCamView.get<ig::TransformComponent>(entity);
            auto& controller = fpsCamView.get<FpsCameraController>(entity);

            if (!bIgnoreInput)
            {
                /* Handle Movements */
                const bool bShouldSprint = sprintAction.IsAnyPressing();
                const float sprintFactor = bShouldSprint ? controller.SprintFactor : 1.f;
                ig::Vector3 direction{};
                if (moveLeftAction.IsAnyPressing())
                {
                    direction += ig::TransformUtility::MakeLeft(transform);
                }

                if (moveRightAction.IsAnyPressing())
                {
                    direction += ig::TransformUtility::MakeRight(transform);
                }

                if (moveForwardAction.IsAnyPressing())
                {
                    direction += ig::TransformUtility::MakeForward(transform);
                }

                if (moveBackwardAction.IsAnyPressing())
                {
                    direction += ig::TransformUtility::MakeBackward(transform);
                }

                if (moveUpAction.IsAnyPressing())
                {
                    direction += ig::Vector3::Up;
                }

                if (moveDownAction.IsAnyPressing())
                {
                    direction += ig::Vector3::Down;
                }

                if (direction != ig::Vector3::Zero)
                {
                    const ig::Vector3 impulse = controller.MovementPower * sprintFactor * direction * deltaTime;

                    controller.LatestImpulse = impulse;
                    controller.ElapsedTimeAfterLatestImpulse = 0.f;
                }

                /* Handle Rotations */
                const float yawAxisValue = turnYawAxis.Value;
                const float pitchAxisValue = turnPitchAxis.Value;
                if (yawAxisValue != 0.f || pitchAxisValue != 0.f)
                {
                    controller.CurrentYaw += yawAxisValue * controller.MouseYawSentisitivity;
                    controller.CurrentPitch += pitchAxisValue * controller.MousePitchSentisitivity;
                    controller.CurrentPitch =
                        std::clamp(controller.CurrentPitch, FpsCameraController::MinPitchDegrees, FpsCameraController::MaxPitchDegrees);
                }
            }

            controller.ElapsedTimeAfterLatestImpulse += deltaTime;
            transform.Position += FPSCameraControllerUtility::CalculateCurrentVelocity(controller);
            transform.Rotation = FPSCameraControllerUtility::CalculateCurrentRotation(controller);
        }
    }
} // namespace fe
