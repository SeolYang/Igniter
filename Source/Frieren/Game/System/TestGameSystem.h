#pragma once
#include "Igniter/Gameplay/GameSystem.h"
#include "Frieren/Game/System/FpsCameraControllSystem.h"
#include "Frieren/Game/System/CameraPossessSystem.h"

namespace ig
{
    class World;
}

namespace fe
{
    class TestGameSystem final : public ig::GameSystem
    {
      public:
        TestGameSystem() = default;
        TestGameSystem(const TestGameSystem&) = delete;
        TestGameSystem(TestGameSystem&&) noexcept = delete;
        ~TestGameSystem() override = default;

        TestGameSystem& operator=(const TestGameSystem&) = delete;
        TestGameSystem& operator=(TestGameSystem&&) noexcept = delete;

        void Update(const float deltaTime, ig::World& world) override;

      private:
        FpsCameraControllSystem fpsCameraControllSystem;
        CameraPossessSystem cameraPossessSystem{fpsCameraControllSystem};
    };

    IG_META_DECLARE(TestGameSystem);
} // namespace fe
