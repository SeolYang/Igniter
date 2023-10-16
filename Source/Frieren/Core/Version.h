#pragma once
#include <cstdint>

namespace fe
{
	enum class Version : uint64_t
	{
	};

	constexpr inline Version CreateVersion(const uint16_t major, const uint16_t minor, const uint32_t patch)
	{
		uint64_t packedVersion = patch;
		packedVersion |= ((static_cast<uint64_t>(major) << 48) | (static_cast<uint64_t>(minor) << 32));
		return (Version)packedVersion;
	}

	constexpr inline uint16_t MajorOf(const Version version)
	{
		const uint64_t packedVersion = static_cast<uint64_t>(version);
		return (packedVersion >> 48) & 0x000000000000ffff;
	}

	constexpr inline uint16_t MinorOf(const Version version)
	{
		const uint64_t packedVersion = static_cast<uint64_t>(version);
		return (packedVersion >> 32) & 0x000000000000ffff;
	}

	constexpr inline uint32_t PatchOf(const Version version)
	{
		const uint64_t packedVersion = static_cast<uint64_t>(version);
		return packedVersion & 0x00000000ffffffff;
	}
} // namespace fe
