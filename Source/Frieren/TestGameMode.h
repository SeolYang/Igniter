#pragma once
#include <memory>
#include <Gameplay/GameMode.h>
#include <FpsCameraControllSystem.h>
#include <CameraPossessSystem.h>

namespace fe
{
    class TestGameMode : public ig::GameMode
    {
    public:
        TestGameMode() = default;
        ~TestGameMode() = default;

        void Update(ig::Registry& registry) override;

    private:
        FpsCameraControllSystem fpsCameraControllSystem;
        CameraPossessSystem cameraPossessSystem{ fpsCameraControllSystem };
    };
}