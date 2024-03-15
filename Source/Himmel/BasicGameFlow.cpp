#include <BasicGameFlow.h>
#include <Gameplay/World.h>
#include <HealthRecoverySystem.h>

BasicGameFlow::BasicGameFlow()
{
}

BasicGameFlow::~BasicGameFlow()
{
}

void BasicGameFlow::Update(fe::World& world)
{
	/* Stateful System **/
	cameraMovementSystem.Update(world);
	rotateEnemySystem.Update(world);
	/* Stateless System **/
	HealthRecoverySystem::Update(world);
}
