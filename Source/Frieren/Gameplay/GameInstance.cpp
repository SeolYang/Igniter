#include <Gameplay/GameInstance.h>
#include <Gameplay/World.h>
#include <Gameplay/GameFlow.h>

namespace fe
{
    GameInstance::GameInstance() {}

    GameInstance::~GameInstance()
    {
        world.reset();
    }

    void GameInstance::Update()
    {
        if (world && gameFlow)
        {
            gameFlow->Update(*world);
        }
    }

    void GameInstance::SetWorld(std::unique_ptr<World> newWorld)
    {
        world = std::move(newWorld);
    }

    void GameInstance::SetGameFlow(std::unique_ptr<GameFlow> newGameFlow)
    {
        gameFlow = std::move(newGameFlow);
    }
} // namespace fe
