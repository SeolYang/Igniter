#include <BasicGameFlow.h>
#include <Gameplay/World.h>
#include <PlayerControllSystem.h>
#include <HealthRecoverySystem.h>

BasicGameFlow::BasicGameFlow()
	: playerControllSystem(std::make_unique<PlayerControllSystem>()), rotateEnemySystem(1.5f)
{
}

BasicGameFlow::~BasicGameFlow()
{
}

void BasicGameFlow::Update(fe::World& world)
{
	/* Stateful System **/
	playerControllSystem->Update(world);
	rotateEnemySystem.Update(world);
	/* Stateless System **/
	HealthRecoverySystem::Update(world);
}
