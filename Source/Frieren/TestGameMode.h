#pragma once
#include <memory>
#include <Gameplay/GameMode.h>
#include <FpsCameraControllSystem.h>
#include <RotateEnemySystem.h>
#include <CameraPossessSystem.h>

class TestGameMode : public ig::GameMode
{
public:
    TestGameMode();
    ~TestGameMode();

    virtual void Update(ig::Registry& registry) override;

private:
    FpsCameraControllSystem fpsCameraControllSystem;
    CameraPossessSystem cameraPossessSystem{ fpsCameraControllSystem };
    RotateEnemySystem rotateEnemySystem;
};