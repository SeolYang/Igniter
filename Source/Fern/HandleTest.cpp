#include <catch.hpp>
#include <Core/HandleManager.h>
#include <Core/Name.h>

struct TestData
{
	std::string nameString;
	fe::Name	name;
};

TEST_CASE("Core.HandleManager")
{
	std::unique_ptr<fe::HandleManager> handleManager = std::make_unique<fe::HandleManager>();
}