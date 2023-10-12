#include <catch.hpp>
#include <Core/Name.h>

TEST_CASE("Core.Name")
{
	using namespace fe;

	constexpr std::string_view testStringView = "프리렌Frieren";
	const std::string		   testString = std::string(testStringView);

	const fe::Name testName = fe::Name(testStringView);
	const fe::Name testNameFromString = fe::Name(testString);
	SECTION("Initialization of Name.")
	{
		REQUIRE(testName.IsValid());
		REQUIRE(testNameFromString.IsValid());
	}

	SECTION("Hash equality of name.")
	{
		REQUIRE(testName == testNameFromString);
		REQUIRE(std::hash<fe::Name>()(testName) == testName.GetHash());
	}

	constexpr std::string_view otherStringView = "페른Fern";
	const fe::Name			   otherName = fe::Name(otherStringView);
	SECTION("Hash inequality of name.")
	{
		REQUIRE(otherName != testName);
		REQUIRE(otherName.GetHash() != testName.GetHash());
	}

	SECTION("String equality of name.")
	{
		REQUIRE(testName.AsString() == testString);
	}

	const fe::Name incorretlyEncodedName = fe::Name(std::string(reinterpret_cast<const char*>(&L"인코딩")));
	const fe::Name invalidEmptyName = fe::Name("");
	SECTION("Invalid Name")
	{
		REQUIRE(!incorretlyEncodedName);
		REQUIRE(!invalidEmptyName);
	}

	SECTION("Inequality of Invalid Name")
	{
		REQUIRE((!(incorretlyEncodedName == invalidEmptyName) && (incorretlyEncodedName != invalidEmptyName)));
	}
}