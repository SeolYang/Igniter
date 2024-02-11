#include <Engine.h>
#include <Core/InputManager.h>
#include <D3D12/Device.h>
#include <Gameplay/GameInstance.h>
#include <Gameplay/World.h>
#include <PlayerArchetype.h>
#include <BasicGameFlow.h>

int main()
{
	int result = 0;
	{
		fe::Engine engine;

		fe::InputManager& inputManager = engine.GetInputManager();
		inputManager.BindAction(fe::String("MoveLeft"), fe::EInput::A);
		inputManager.BindAction(fe::String("MoveRight"), fe::EInput::D);
		inputManager.BindAction(fe::String("UseHealthRecovery"), fe::EInput::Space);
		inputManager.BindAction(fe::String("DisplayPlayerInfo"), fe::EInput::MouseLB);

		fe::GameInstance&		   gameInstance = engine.GetGameInstance();
		std::unique_ptr<fe::World> defaultWorld = std::make_unique<fe::World>();
		PlayerArchetype::Create(*defaultWorld);
		gameInstance.SetWorld(std::move(defaultWorld));
		gameInstance.SetGameFlow(std::make_unique<BasicGameFlow>());

		result = engine.Execute();
	}

	return result;
}