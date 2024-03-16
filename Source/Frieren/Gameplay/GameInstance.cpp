#include <Gameplay/GameInstance.h>
#include <Gameplay/GameMode.h>

namespace fe
{
    GameInstance::GameInstance() {}

    GameInstance::~GameInstance()
    {
    }

    void GameInstance::Update()
    {
        if (gameMode)
        {
            gameMode->Update(registry);
        }
    }

    void GameInstance::SetGameMode(std::unique_ptr<GameMode> newGameMode)
    {
        gameMode = std::move(newGameMode);
    }
} // namespace fe
