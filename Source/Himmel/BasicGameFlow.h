#pragma once
#include <memory>
#include <Gameplay/GameFlow.h>
#include <CameraMovementSystem.h>
#include <RotateEnemySystem.h>
#include <CameraPossessSystem.h>

class BasicGameFlow : public fe::GameFlow
{
public:
    BasicGameFlow();
    ~BasicGameFlow();

    virtual void Update(fe::World& world) override;

private:
	CameraMovementSystem cameraMovementSystem;
	CameraPossessSystem cameraPossessSystem{ cameraMovementSystem };
	RotateEnemySystem rotateEnemySystem;
};