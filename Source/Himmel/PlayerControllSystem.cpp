#include <PlayerControllSystem.h>
#include <Engine.h>
#include <Core/Timer.h>
#include <Core/Log.h>
#include <Gameplay/World.h>
#include <Render/TransformComponent.h>
#include <HealthComponent.h>
#include <HealthRecoveryBuff.h>
#include <PlayerComponent.h>
#include <ControllableTag.h>

PlayerControllSystem::PlayerControllSystem()
	: timer(fe::Engine::GetTimer())
{
	auto& inputManager = fe::Engine::GetInputManager();
	moveLeftAction = inputManager.QueryAction(fe::String("MoveLeft"));
	moveRightAction = inputManager.QueryAction(fe::String("MoveRight"));
	healthRecoveryAction = inputManager.QueryAction(fe::String("UseHealthRecovery"));
	displayPlayerInfoAction = inputManager.QueryAction(fe::String("DisplayPlayerInfo"));
}

void PlayerControllSystem::Update(fe::World& world)
{
	HandleMoveAction(world);
	HandleUseHealthRecoveryAction(world);
	HandleDisplayPlayerInfoAction(world);
}

void PlayerControllSystem::HandleMoveAction(fe::World& world)
{
	if (moveLeftAction)
	{
		if (moveLeftAction->IsAnyPressing())
		{
			world.Each<Player, fe::TransformComponent, Controllable>(
				[this](Player& player, fe::TransformComponent& transform) {
					transform.Position.x -= player.movementPower * timer.GetDeltaTime();
				});
		}
	}

	if (moveRightAction)
	{
		if (moveRightAction->IsAnyPressing())
		{
			world.Each<Player, Controllable, fe::TransformComponent>(
				[this](Player& player, fe::TransformComponent& transform)
				{
					transform.Position.x += player.movementPower * timer.GetDeltaTime();
				});
		}
	}
}

void PlayerControllSystem::HandleUseHealthRecoveryAction(fe::World& world)
{
	if (healthRecoveryAction)
	{
		if (healthRecoveryAction->State == fe::EInputState::Pressed)
		{
			world.Each<Player, Controllable, HealthComponent>(
				[&world](const fe::Entity& entity, Player& player, HealthComponent& health) 
				{
					if (player.remainHealthRecoveryBuff > 0 && health.value < HealthComponent::Maximum)
					{
						--player.remainHealthRecoveryBuff;
						FE_LOG(fe::LogInfo, "HP Recovery Buff Activated! Remain buff counter {}", player.remainHealthRecoveryBuff);
						world.Attach<HealthRecoveryBuff>(entity);
					}
				}, entt::exclude<HealthRecoveryBuff>);
		}
	}
}

void PlayerControllSystem::HandleDisplayPlayerInfoAction(fe::World& world)
{
	if (displayPlayerInfoAction)
	{
		if (displayPlayerInfoAction->State == fe::EInputState::Pressed)
		{
			world.Each<Player, HealthComponent, fe::TransformComponent>(
				[](const Player& player, const HealthComponent& healthComponent, const fe::TransformComponent& transform)
				{
					FE_LOG(fe::LogInfo, "Health Recovery Buff remains: {}, Health: {}", player.remainHealthRecoveryBuff, healthComponent.value);
					FE_LOG(fe::LogInfo, "Transform.Pos.X: {}", transform.Position.x);
				});
		}
	}
}
