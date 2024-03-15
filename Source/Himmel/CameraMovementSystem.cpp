#include <CameraMovementSystem.h>
#include <Render/TransformComponent.h>
#include <Render/CameraComponent.h>
#include <Core/InputManager.h>
#include <Core/Log.h>
#include <Gameplay/World.h>
#include <Engine.h>
#include <CameraController.h>

CameraMovementSystem::CameraMovementSystem()
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

void CameraMovementSystem::Update(fe::World& world)
{
	if (!bIgnoreInput)
	{
		if (sprintAction)
		{
			bSprint = sprintAction->IsAnyPressing();
		}

		if (moveLeftAction)
		{
			if (moveLeftAction->IsAnyPressing())
			{
				world.Each<fe::TransformComponent, CameraController>(
					[this](fe::TransformComponent& transform, CameraController& controller)
					{
						fe::Vector3 vector = transform.GetLeftDirection() * controller.MovementPower * timer.GetDeltaTime();
						if (bSprint)
						{
							vector *= controller.SprintFactor;
						}

						transform.Position += vector;
					});
			}
		}

		if (moveRightAction)
		{
			if (moveRightAction->IsAnyPressing())
			{
				world.Each<fe::TransformComponent, CameraController>(
					[this](fe::TransformComponent& transform, CameraController& controller)
					{
						fe::Vector3 vector = transform.GetRightDirection() * controller.MovementPower * timer.GetDeltaTime();
						if (bSprint)
						{
							vector *= controller.SprintFactor;
						}

						transform.Position += vector;
					});
			}
		}

		if (moveForwardAction)
		{
			if (moveForwardAction->IsAnyPressing())
			{
				world.Each<fe::TransformComponent, CameraController>(
					[this](fe::TransformComponent& transform, CameraController& controller)
					{
						fe::Vector3 vector = transform.GetForwardDirection() * controller.MovementPower * timer.GetDeltaTime();
						if (bSprint)
						{
							vector *= controller.SprintFactor;
						}

						transform.Position += vector;
					});
			}
		}

		if (moveBackwardAction)
		{
			if (moveBackwardAction->IsAnyPressing())
			{
				world.Each<fe::TransformComponent, CameraController>(
					[this](fe::TransformComponent& transform, CameraController& controller)
					{
						fe::Vector3 vector = transform.GetBackwardDirection() * controller.MovementPower * timer.GetDeltaTime();
						if (bSprint)
						{
							vector *= controller.SprintFactor;
						}

						transform.Position += vector;
					});
			}
		}

		if (moveUpAction)
		{
			if (moveUpAction->IsAnyPressing())
			{
				world.Each<fe::TransformComponent, CameraController>(
					[this](fe::TransformComponent& transform, CameraController& controller)
					{
						fe::Vector3 vector = fe::Vector3::Up * controller.MovementPower * timer.GetDeltaTime();
						if (bSprint)
						{
							vector *= controller.SprintFactor;
						}

						transform.Position += vector;
					});
			}
		}

		if (moveDownAction)
		{
			if (moveDownAction->IsAnyPressing())
			{
				world.Each<fe::TransformComponent, CameraController>(
					[this](fe::TransformComponent& transform, CameraController& controller)
					{
						fe::Vector3 vector = fe::Vector3::Down * controller.MovementPower * timer.GetDeltaTime();
						if (bSprint)
						{
							vector *= controller.SprintFactor;
						}

						transform.Position += vector;
					});
			}
		}

		if (turnYawAxis)
		{
			const float value = turnYawAxis->Value;
			if (value != 0.f)
			{
				yawDegrees += value * mouseYawSentisitivity;
				bTurnDirty = true;
			}
		}

		if (turnPitchAxis)
		{
			const float value = turnPitchAxis->Value;
			if (value != 0.f)
			{
				pitchDegrees += value * mousePitchSentisitivity;
				bTurnDirty = true;
			}
		}

		if (bTurnDirty)
		{
			bTurnDirty = false;
			world.Each<fe::TransformComponent, CameraController>(
				[this](fe::TransformComponent& transform, CameraController&)
				{
					transform.Rotation = fe::Quaternion::CreateFromYawPitchRoll(fe::Deg2Rad(yawDegrees), fe::Deg2Rad(pitchDegrees), 0.f);
				});
		}
	}
}
