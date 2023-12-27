#include <Gameplay/World.h>

namespace fe
{
	void World::Destroy(const Entity entity)
	{
		registry.destroy(entity);
	}

	bool World::IsOrphan(const Entity entity) const
	{
		return registry.orphan(entity);
	}

	bool World::IsValid(const Entity entity) const
	{
		return registry.valid(entity);
	}
} // namespace fe
