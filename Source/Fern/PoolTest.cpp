#include <catch.hpp>
#include <Core/Pool.h>
#include <Core/String.h>

TEST_CASE("Core.Pool")
{
    using namespace fe;
	constexpr size_t NumOfInitialChunks = 1;
	constexpr size_t NumOfElementsPerChunk = 2;
    fe::Pool<String> pool{ NumOfInitialChunks, NumOfElementsPerChunk };

    SECTION("Allocation-Deallocation")
    {
		PoolAllocation<String> a0 = pool.Allocate("Test0");
		REQUIRE(a0.ChunkIndex == 0);
		REQUIRE(a0.ElementIndex == 0);
		REQUIRE((*pool.GetAddressOfAllocation(a0)) == "Test0");

		PoolAllocation<String> a1 = pool.Allocate("Test1");
		REQUIRE(a1.ChunkIndex == 0);
		REQUIRE(a1.ElementIndex == 1);
		REQUIRE((*pool.GetAddressOfAllocation(a1)) == "Test1");

		PoolAllocation<String> a2 = pool.Allocate("Test2");
		REQUIRE(a2.ChunkIndex == 1);
		REQUIRE(a2.ElementIndex == 0);
		REQUIRE((*pool.GetAddressOfAllocation(a2)) == "Test2");

		pool.SafeDeallocate(a1);
		PoolAllocation<String> a3 = pool.Allocate("Test3");
		REQUIRE(a3.ChunkIndex == 0);
		REQUIRE(a3.ElementIndex == 1);
		REQUIRE((*pool.GetAddressOfAllocation(a3)) == "Test3");
		REQUIRE(pool.GetAddressOfAllocation(a1) ==  nullptr);

		pool.SafeDeallocate(a0);
		pool.SafeDeallocate(a2);
		pool.SafeDeallocate(a3);
    }
}