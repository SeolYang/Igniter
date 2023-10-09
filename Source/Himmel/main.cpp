#include <iostream>
#include <Core/String.h>
#include <Core/Hash.h>
#include <Core/Assert.h>
#include <Core/Name.h>
#include <Core/Pool.h>

int main()
{
	constexpr std::string_view compileTimeStringView = FE_TEXT("테스트");
	constexpr uint64_t		   compileTimeHashTest = fe::HashCRC64(compileTimeStringView);
	constexpr uint64_t		   compileTimeHasOfType = fe::HashOfType<std::string_view>;

	std::string	   runtimeString = FE_TEXT("테스트");
	const uint64_t runtimeHashTest = fe::HashCRC64(runtimeString);

	FE_ASSERT(runtimeHashTest == compileTimeHashTest, "");

	const std::string nameString = FE_TEXT("프리렌");
	const fe::Name	  name = fe::Name(nameString);
	FE_ASSERT(name.IsValid());
	FE_ASSERT(nameString == name.AsString());
	FE_ASSERT(std::hash<fe::Name>()(name) == name.GetHash());

	// fe::Name invalidName = fe::Name(std::string(reinterpret_cast<const char*>(&L"인코딩")));
	// fe::Name invalidEmptyName = fe::Name("");
	// FE_ASSERT(invalidName != invalidEmptyName);

	fe::Pool<unsigned int> pool{ 1, 1 };
	auto				   allocation = pool.Allocate(0);
	auto				   allocation2 = pool.Allocate(1);
	pool.Deallocate(allocation);
	pool.Deallocate(allocation2);

	return 0;
}