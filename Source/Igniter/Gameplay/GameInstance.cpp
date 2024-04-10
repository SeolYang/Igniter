#include <Igniter.h>
#include <Gameplay/GameInstance.h>
#include <Gameplay/GameMode.h>

namespace ig
{
    GameInstance::GameInstance() {}

    GameInstance::~GameInstance()
    {
        registry.clear();
    }

    void GameInstance::Update()
    {
        ZoneScoped;
        if (gameMode)
        {
            gameMode->Update(registry);
        }
    }

    void GameInstance::SetGameMode(std::unique_ptr<GameMode> newGameMode)
    {
        gameMode = std::move(newGameMode);
    }
} // namespace ig
