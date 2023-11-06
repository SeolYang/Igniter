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

	fe::WeakHandle<const fe::Action> moveLeftAction;
	fe::WeakHandle<const fe::Action> moveRightAction;
	fe::WeakHandle<const fe::Action> healthRecoveryAction;
	fe::WeakHandle<const fe::Action> displayPlayerInfoAction;
};
