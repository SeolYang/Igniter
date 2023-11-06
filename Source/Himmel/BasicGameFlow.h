#pragma once
#include <memory>
#include <Gameplay/GameFlow.h>

class PlayerControllSystem;
class BasicGameFlow : public fe::GameFlow
{
public:
	BasicGameFlow();
	~BasicGameFlow();

	virtual void Update(fe::World& world) override;

	private:
	std::unique_ptr<PlayerControllSystem> playerControllSystem;
};