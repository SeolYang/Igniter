#pragma once
#include <cstdint>
#include <string_view>

namespace fe
{
	namespace version
	{
		constexpr uint8_t		   Major = 0;
		constexpr uint8_t		   Minor = 1;
		constexpr uint8_t		   Patch = 2;
		constexpr std::string_view Version = "0.1.2";
	} // namespace version
} // namespace fe
