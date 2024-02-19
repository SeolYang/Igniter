#pragma once
#include <Core/InputManager.h>

namespace fe
{
	class World;
	class Timer;
} // namespace fe

class PlayerControllSystem
{
public:
	PlayerControllSystem();

	void Update(fe::World& world);

private:
	void HandleMoveAction(fe::World& world);
	void HandleUseHealthRecoveryAction(fe::World& world);
	void HandleDisplayPlayerInfoAction(fe::World& world);

private:
	const fe::Timer& timer;

	fe::WeakHandle<fe::Action> moveLeftAction;
	fe::WeakHandle<fe::Action> moveRightAction;
	fe::WeakHandle<fe::Action> healthRecoveryAction;
	fe::WeakHandle<fe::Action> displayPlayerInfoAction;
};
