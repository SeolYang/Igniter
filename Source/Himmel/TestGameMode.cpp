#include <TestGameMode.h>
#include <HealthRecoverySystem.h>

TestGameMode::TestGameMode()
{
}

TestGameMode::~TestGameMode()
{
}

void TestGameMode::Update(fe::Registry& registry)
{
    /* Stateful System **/
    fpsCameraControllSystem.Update(registry);
    cameraPossessSystem.Update();
    rotateEnemySystem.Update(registry);
    /* Stateless System **/
    HealthRecoverySystem::Update(registry);
}
