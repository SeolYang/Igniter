#include <catch.hpp>
#include <Core/String.h>

TEST_CASE("Core.String")
{
	using namespace fe;

	constexpr std::string_view testStringView = "프리렌Frieren";
	const std::string		   testString = std::string(testStringView);

	const fe::String testName = fe::String(testStringView);
	const fe::String testNameFromString = fe::String(testString);
	SECTION("Initialization of String.")
	{
		REQUIRE(testName.IsValid());
		REQUIRE(testNameFromString.IsValid());
	}

	SECTION("Hash equality of name.")
	{
		REQUIRE(testName == testNameFromString);
		REQUIRE(std::hash<fe::String>()(testName) == testName.GetHash());
	}

	constexpr std::string_view otherStringView = "페른Fern";
	const fe::String		   otherName = fe::String(otherStringView);
	SECTION("Hash inequality of name.")
	{
		REQUIRE(otherName != testName);
		REQUIRE(otherName.GetHash() != testName.GetHash());
	}

	SECTION("String equality of name.")
	{
		REQUIRE(testName.AsString() == testString);
	}

	const fe::String incorretlyEncodedName = fe::String(std::string(reinterpret_cast<const char*>(&L"인코딩")));
	const fe::String invalidEmptyName = fe::String("");
	SECTION("Invalid String")
	{
		REQUIRE(!incorretlyEncodedName);
		REQUIRE(!invalidEmptyName);
	}

	SECTION("Inequality of Invalid String")
	{
		REQUIRE((!(incorretlyEncodedName == invalidEmptyName) && (incorretlyEncodedName != invalidEmptyName)));
	}

	const std::wstring wideCharString = L"안녕";
	const fe::String   feStringHello = fe::String("안녕");
	SECTION("String As Wide Character String")
	{
		REQUIRE(wideCharString == feStringHello.AsWideString());
	}
	
}