#include <Frieren.h>
#include <Game/System/FpsCameraControllSystem.h>
#include <Component/TransformComponent.h>
#include <Component/CameraComponent.h>
#include <Input/InputManager.h>
#include <Core/Log.h>
#include <Core/Engine.h>
#include <Core/Timer.h>
#include <Gameplay/World.h>
#include <Game/Component/FpsCameraController.h>

namespace fe
{
    FpsCameraControllSystem::FpsCameraControllSystem() : timer(ig::Igniter::GetTimer())
    {
        auto& inputManager = ig::Igniter::GetInputManager();
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

    void FpsCameraControllSystem::Update(ig::World& world)
    {
        Registry& registry = world.GetRegistry();
        const auto& inputManager = Igniter::GetInputManager();
        const Action moveLeftAction = inputManager.GetAction(moveLeftActionHandle);
        const Action moveRightAction = inputManager.GetAction(moveRightActionHandle);
        const Action moveForwardAction = inputManager.GetAction(moveForwardActionHandle);
        const Action moveBackwardAction = inputManager.GetAction(moveBackwardActionHandle);
        const Action moveUpAction = inputManager.GetAction(moveUpActionHandle);
        const Action moveDownAction = inputManager.GetAction(moveDownActionHandle);

        const Axis turnYawAxis = inputManager.GetAxis(turnYawAxisHandle);
        const Axis turnPitchAxis = inputManager.GetAxis(turnPitchAxisHandle);

        const Action sprintAction = inputManager.GetAction(sprintActionHandle);

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
                    direction += transform.GetLeftDirection();
                }

                if (moveRightAction.IsAnyPressing())
                {
                    direction += transform.GetRightDirection();
                }

                if (moveForwardAction.IsAnyPressing())
                {
                    direction += transform.GetForwardDirection();
                }

                if (moveBackwardAction.IsAnyPressing())
                {
                    direction += transform.GetBackwardDirection();
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
                    const ig::Vector3 impulse = controller.MovementPower * sprintFactor * direction * timer.GetDeltaTime();

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

            controller.ElapsedTimeAfterLatestImpulse += timer.GetDeltaTime();
            transform.Position += controller.GetCurrentVelocity();
            transform.Rotation = controller.GetCurrentRotation();
        }
    }
}    // namespace fe
