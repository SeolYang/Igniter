#pragma once
#include <memory>
#include <Gameplay/GameFlow.h>
#include <CameraMovementSystem.h>
#include <RotateEnemySystem.h>

class BasicGameFlow : public fe::GameFlow
{
public:
    BasicGameFlow();
    ~BasicGameFlow();

    virtual void Update(fe::World& world) override;

private:
	CameraMovementSystem cameraMovementSystem;
	RotateEnemySystem rotateEnemySystem;
};