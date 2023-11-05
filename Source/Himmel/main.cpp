#include <Engine.h>
#include <entt/entt.hpp>
int main()
{
	int result = 0;
	{
		fe::Engine engine;
		result = engine.Execute();
	}

	entt::registry registry;
	return result;
}