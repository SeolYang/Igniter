#include "Frieren/Frieren.h"
#include "Igniter/Gameplay/World.h"
#include "Frieren/Game/System/TestGameSystem.h"

namespace fe
{
    void TestGameSystem::Update(const float deltaTime, ig::World& world)
    {
        fpsCameraControllSystem.Update(deltaTime, world);
        cameraPossessSystem.Update();
    }

    template <>
    void DefineMeta<TestGameSystem>() {}

    IG_DEFINE_TYPE_META_AS_GAME_SYSTEM(TestGameSystem);
} // namespace fe
