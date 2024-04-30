#pragma once
#include <Gameplay/GameMode.h>
#include <Game/System/FpsCameraControllSystem.h>
#include <Game/System/CameraPossessSystem.h>

namespace ig
{
    class World;
}

namespace fe
{
    class TestGameMode final : public ig::GameMode
    {
    public:
        TestGameMode() = default;
        TestGameMode(const TestGameMode&) = delete;
        TestGameMode(TestGameMode&&) noexcept = delete;
        ~TestGameMode() override = default;

        TestGameMode& operator=(const TestGameMode&) = delete;
        TestGameMode& operator=(TestGameMode&&) noexcept = delete;

        void Update(ig::World& world) override;

    private:
        FpsCameraControllSystem fpsCameraControllSystem;
        CameraPossessSystem cameraPossessSystem{fpsCameraControllSystem};
    };

    IG_DECLARE_TYPE_META(TestGameMode);
}    // namespace fe
