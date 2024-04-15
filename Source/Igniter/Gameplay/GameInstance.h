#pragma once
#include <Igniter.h>

namespace ig
{
    class GameMode;

    class GameInstance
    {
    public:
        GameInstance();
        ~GameInstance();

        GameInstance(const GameInstance&)     = delete;
        GameInstance(GameInstance&&) noexcept = delete;

        GameInstance& operator=(const GameInstance&) = delete;
        GameInstance& operator=(GameInstance&&)      = delete;

        Registry& GetRegistry() { return registry; }

        void Update();

        void SetGameMode(std::unique_ptr<GameMode> newGameMode);

        OptionalRef<GameMode> GetGameMode()
        {
            return gameMode != nullptr ? MakeOptionalRef(*gameMode) : std::nullopt;
        }

    private:
        Registry                  registry;
        std::unique_ptr<GameMode> gameMode;
    };
} // namespace ig
