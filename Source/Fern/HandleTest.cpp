#include <catch.hpp>
#include <Core/HandleManager.h>
#include <Core/Name.h>

struct TestData
{
	std::string nameString;
	fe::Name	name;

	~TestData()
	{
		nameString.clear();
	}
};

TEST_CASE("Core.HandleManager")
{
	using namespace fe;
	HandleManager handleManager;

	const TestData td0{
		"NiceString",
		Name{ "NiceName" }
	};

	const TestData td1{
		"Fern, The Test Project",
		Name{ "FErn" }
	};

	OwnedHandle<TestData> h0 = OwnedHandle<TestData>::Create(
		handleManager,
		"TD0",
		td0);

	OwnedHandle<TestData> h1 = OwnedHandle<TestData>::Create(
		handleManager,
		"TD1",
		td1);

	SECTION("Creation of Handle and construction of instances")
	{
		REQUIRE(h0.IsValid());
		REQUIRE(h0);
		REQUIRE(h1.IsValid());
		REQUIRE(h0 != h1);

		// name query 과정에 뭔가 이상함
		auto queriedName = h0.QueryName();
		REQUIRE(h0.QueryName() == Name("TD0"));

		REQUIRE(h0->name == td0.name);
		REQUIRE(h0->nameString == td0.nameString);


		REQUIRE(h1.QueryName() == Name("TD1"));
	}

	WeakHandle<TestData> wh0_0 = h0.DeriveWeak();
	WeakHandle<TestData> wh0_1 = h0.DeriveWeak();
	WeakHandle<TestData> wh1_0 = h1.DeriveWeak();
	SECTION("Derive Weak Handle from Owned Handle")
	{
		REQUIRE(wh0_0);
		REQUIRE(wh0_1);
		REQUIRE(wh0_0 == h0);
		REQUIRE(wh0_1 == h0);
		REQUIRE(wh0_0 == wh0_1);
		REQUIRE(wh1_0 != wh0_0);
		REQUIRE(wh1_0 != h0);
		REQUIRE(wh1_0 == h1);

		REQUIRE(wh0_0.QueryName() == Name("TD0"));

		REQUIRE(wh0_0->name == td0.name);
		REQUIRE(wh0_0->nameString == td0.nameString);
		REQUIRE(wh0_0->name == wh0_1->name);
		REQUIRE(wh0_0->nameString == wh0_1->nameString);
	}

	SECTION("Renaming Handle")
	{
		wh0_0.Rename(FE_NAME("Renamed TD0"));
		REQUIRE(wh0_0.QueryName() == FE_NAME("Renamed TD0"));
		REQUIRE(h0.QueryName() == FE_NAME("Renamed TD0"));
	}

	SECTION("Destroy-Expired Handle")
	{
		h0.Destroy();
		REQUIRE(!h0);
		REQUIRE(!wh0_0);
		REQUIRE(!wh0_1);

		h1.Destroy();
		REQUIRE(!h1);
		REQUIRE(!wh1_0);

		REQUIRE(!h0.QueryName());
		REQUIRE(!h1.QueryName());
		REQUIRE(!wh0_0.QueryName());
		REQUIRE(!wh0_1.QueryName());
		REQUIRE(!wh1_0.QueryName());

		fe::WeakHandle<TestData> weakHandleFromExpired = h0.DeriveWeak();
		REQUIRE(!weakHandleFromExpired);
	}
}