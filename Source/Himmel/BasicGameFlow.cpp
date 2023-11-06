#include <BasicGameFlow.h>
#include <Gameplay/World.h>
#include <PlayerControllSystem.h>
#include <HealthRecoverySystem.h>

BasicGameFlow::BasicGameFlow()
	: playerControllSystem(std::make_unique<PlayerControllSystem>())
{
}

BasicGameFlow::~BasicGameFlow()
{
}

void BasicGameFlow::Update(fe::World& world)
{
	/* Stateful System **/
	playerControllSystem->Update(world);
	/* Stateless System **/
	HealthRecoverySystem::Update(world);
}
