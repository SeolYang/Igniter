#include <Frieren.h>
#include <Game/System/FpsCameraControllSystem.h>
#include <Component/TransformComponent.h>
#include <Component/CameraComponent.h>
#include <Input/InputManager.h>
#include <Core/Log.h>
#include <Core/Engine.h>
#include <Core/Timer.h>
#include <Game/Component/FpsCameraController.h>

namespace fe
{
    FpsCameraControllSystem::FpsCameraControllSystem()
        : timer(ig::Igniter::GetTimer())
    {
        auto& inputManager = ig::Igniter::GetInputManager();
        moveLeftAction = inputManager.QueryAction(ig::String("MoveLeft"));
        moveRightAction = inputManager.QueryAction(ig::String("MoveRight"));
        moveForwardAction = inputManager.QueryAction(ig::String("MoveForward"));
        moveBackwardAction = inputManager.QueryAction(ig::String("MoveBackward"));

        moveUpAction = inputManager.QueryAction(ig::String("MoveUp"));
        moveDownAction = inputManager.QueryAction(ig::String("MoveDown"));

        sprintAction = inputManager.QueryAction(ig::String("Sprint"));

        turnYawAxis = inputManager.QueryAxis(ig::String("TurnYaw"));
        turnPitchAxis = inputManager.QueryAxis(ig::String("TurnAxis"));
    }

    void FpsCameraControllSystem::Update(ig::Registry& registry)
    {
        const auto fpsCamView = registry.view<ig::TransformComponent, FpsCameraController, ig::CameraComponent>();
        for (const ig::Entity entity : fpsCamView)
        {
            auto& transform = fpsCamView.get<ig::TransformComponent>(entity);
            auto& controller = fpsCamView.get<FpsCameraController>(entity);

            if (!bIgnoreInput)
            {
                /* Handle Movements */
                const bool bShouldSprint = sprintAction && sprintAction->IsAnyPressing();
                const float sprintFactor = bShouldSprint ? controller.SprintFactor : 1.f;
                ig::Vector3 direction{};
                if (moveLeftAction && moveLeftAction->IsAnyPressing())
                {
                    direction += transform.GetLeftDirection();
                }

                if (moveRightAction && moveRightAction->IsAnyPressing())
                {
                    direction += transform.GetRightDirection();
                }

                if (moveForwardAction && moveForwardAction->IsAnyPressing())
                {
                    direction += transform.GetForwardDirection();
                }

                if (moveBackwardAction && moveBackwardAction->IsAnyPressing())
                {
                    direction += transform.GetBackwardDirection();
                }

                if (moveUpAction && moveUpAction->IsAnyPressing())
                {
                    direction += ig::Vector3::Up;
                }

                if (moveDownAction && moveDownAction->IsAnyPressing())
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
                const float yawAxisValue = turnYawAxis ? turnYawAxis->Value : 0.f;
                const float pitchAxisValue = turnPitchAxis ? turnPitchAxis->Value : 0.f;
                if (yawAxisValue != 0.f || pitchAxisValue != 0.f)
                {
                    controller.CurrentYaw += yawAxisValue * controller.MouseYawSentisitivity;
                    controller.CurrentPitch += pitchAxisValue * controller.MousePitchSentisitivity;
                    controller.CurrentPitch = std::clamp(controller.CurrentPitch, FpsCameraController::MinPitchDegrees, FpsCameraController::MaxPitchDegrees);
                }
            }

            controller.ElapsedTimeAfterLatestImpulse += timer.GetDeltaTime();
            transform.Position += controller.GetCurrentVelocity();
            transform.Rotation = controller.GetCurrentRotation();
        }
    }
} // namespace fe