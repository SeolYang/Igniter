#include <TestGameMode.h>

namespace fe
{
    void TestGameMode::Update(ig::Registry& registry)
    {
        fpsCameraControllSystem.Update(registry);
        cameraPossessSystem.Update();
    }
} // namespace fe
