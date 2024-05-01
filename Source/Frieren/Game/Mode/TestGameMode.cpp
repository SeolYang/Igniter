#include "Frieren/Frieren.h"
#include "Igniter/Gameplay/World.h"
#include "Frieren/Game/Mode/TestGameMode.h"

namespace fe
{
    void TestGameMode::Update(ig::World& world)
    {
        fpsCameraControllSystem.Update(world);
        cameraPossessSystem.Update();
    }

    template<>
    void DefineMeta<TestGameMode>()
    {
    }

    IG_DEFINE_TYPE_META_AS_GAME_MODE(TestGameMode);
}    // namespace fe
