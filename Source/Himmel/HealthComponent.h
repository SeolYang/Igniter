#pragma once

struct HealthComponent
{
	constexpr static float Default = 10.f;
	constexpr static float Maximum = 100.f;

	float value = Default;
};