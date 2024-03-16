#include <FpsCameraControllSystem.h>
#include <Render/TransformComponent.h>
#include <Render/CameraComponent.h>
#include <Core/InputManager.h>
#include <Core/Log.h>
#include <Engine.h>
#include <FpsCameraController.h>

FpsCameraControllSystem::FpsCameraControllSystem()
    : timer(fe::Engine::GetTimer())
{
    auto& inputManager = fe::Engine::GetInputManager();
    moveLeftAction = inputManager.QueryAction(fe::String("MoveLeft"));
    moveRightAction = inputManager.QueryAction(fe::String("MoveRight"));
    moveForwardAction = inputManager.QueryAction(fe::String("MoveForward"));
    moveBackwardAction = inputManager.QueryAction(fe::String("MoveBackward"));

    moveUpAction = inputManager.QueryAction(fe::String("MoveUp"));
    moveDownAction = inputManager.QueryAction(fe::String("MoveDown"));

    sprintAction = inputManager.QueryAction(fe::String("Sprint"));

    turnYawAxis = inputManager.QueryAxis(fe::String("TurnYaw"));
    turnPitchAxis = inputManager.QueryAxis(fe::String("TurnAxis"));
}

void FpsCameraControllSystem::Update(fe::Registry& registry)
{
    const auto fpsCamView = registry.view<fe::TransformComponent, FpsCameraController, fe::CameraComponent>();
    for (const fe::Entity entity : fpsCamView)
    {
        auto& transform = fpsCamView.get<fe::TransformComponent>(entity);
        auto& controller = fpsCamView.get<FpsCameraController>(entity);

        if (!bIgnoreInput)
        {
            /* Handle Movements */
            const bool bShouldSprint = sprintAction && sprintAction->IsAnyPressing();
            const float sprintFactor = bShouldSprint ? controller.SprintFactor : 1.f;
            fe::Vector3 direction{};
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
                direction += fe::Vector3::Up;
            }

            if (moveDownAction && moveDownAction->IsAnyPressing())
            {
                direction += fe::Vector3::Down;
            }

            if (direction != fe::Vector3::Zero)
            {
                const fe::Vector3 impulse = controller.MovementPower * sprintFactor * direction * timer.GetDeltaTime();
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
            }
        }

        controller.ElapsedTimeAfterLatestImpulse += timer.GetDeltaTime();
        transform.Position += controller.GetCurrentVelocity();
        transform.Rotation = controller.GetCurrentRotation();
    }
}
