#pragma once
#include <Core/Version.h>
#include <string_view>

namespace fe::settings
{
	constexpr Version GameVersion = CreateVersion(0, 1, 0);
	constexpr std::string_view GameName = "Untitled";
}