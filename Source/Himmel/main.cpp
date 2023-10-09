#include <iostream>
#include <Core/String.h>
#include <Core/Hash.h>
#include <Core/Assert.h>

int main()
{
	constexpr std::string_view compileTimeStringView = FE_TEXT("테스트");
	constexpr uint64_t		   compileTimeHashTest = fe::HashCRC64(compileTimeStringView);
	constexpr uint64_t		   compileTimeHasOfType = fe::HashOfType<std::string_view>;

	std::string runtimeString = FE_TEXT("테스트");
	const uint64_t runtimeHashTest = fe::HashCRC64(runtimeString);

	FE_ASSERT(runtimeHashTest == compileTimeHashTest, "");
    return 0;
}