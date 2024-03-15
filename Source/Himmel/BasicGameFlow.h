#pragma once
#include <memory>
#include <Gameplay/GameFlow.h>
#include <FpsCameraControllSystem.h>
#include <RotateEnemySystem.h>
#include <CameraPossessSystem.h>

class BasicGameFlow : public fe::GameFlow
{
public:
    BasicGameFlow();
    ~BasicGameFlow();

    virtual void Update(fe::World& world) override;

private:
    FpsCameraControllSystem fpsCameraControllSystem;
    CameraPossessSystem cameraPossessSystem{ fpsCameraControllSystem };
    RotateEnemySystem rotateEnemySystem;
};