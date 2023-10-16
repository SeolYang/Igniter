#include <catch.hpp>
#include <Core/Version.h>

TEST_CASE("Core.Version")
{
	const fe::Version version = fe::CreateVersion(135, 223, 3111);
	REQUIRE(MajorOf(version) == 135);
	REQUIRE(MinorOf(version) == 223);
	REQUIRE(PatchOf(version) == 3111);
}