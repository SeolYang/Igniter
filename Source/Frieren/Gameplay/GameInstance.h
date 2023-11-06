#pragma once
#include <memory>

namespace fe
{
	class GameFlow;
	class World;
	class GameInstance final
	{
	public:
		GameInstance();
		~GameInstance();

		GameInstance(const GameInstance&) = delete;
		GameInstance(GameInstance&&) noexcept = delete;

		GameInstance& operator=(const GameInstance&) = delete;
		GameInstance& operator=(GameInstance&&) = delete;

		void Update();

		void SetWorld(std::unique_ptr<World> world);
		void SetGameFlow(std::unique_ptr<GameFlow> gameFlow);

	private:
		std::unique_ptr<World>	  world;
		std::unique_ptr<GameFlow> gameFlow;
	};
} // namespace fe