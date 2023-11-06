#pragma once

namespace fe
{
	class World;
}

class HealthRecoverySystem
{
public:
	static void Update(fe::World& world);
};