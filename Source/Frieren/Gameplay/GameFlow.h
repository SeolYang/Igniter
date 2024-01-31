#pragma once

namespace fe
{
	class World;
	class GameFlow
	{
	public:
		GameFlow() = default;
		virtual ~GameFlow() = default;
		virtual void Update(World& world) = 0;
	};
} // namespace fe