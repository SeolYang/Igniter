#include <iostream>
#include <Core/String.h>
#include <Core/Hash.h>
#include <Core/Assert.h>
#include <Core/Name.h>
#include <Core/Pool.h>
#include <Core/HandleManager.h>

int main()
{
	constexpr std::string_view compileTimeStringView = "테스트";
	constexpr uint64_t		   compileTimeHashTest = fe::HashCRC64(compileTimeStringView);
	constexpr uint64_t		   compileTimeHasOfType = fe::HashOfType<std::string_view>;

	std::string	   runtimeString = "테스트";
	const uint64_t runtimeHashTest = fe::HashCRC64(runtimeString);

	FE_ASSERT(runtimeHashTest == compileTimeHashTest, "");

	const std::string nameString = "프리렌";
	const fe::Name	  name = fe::Name(nameString);
	FE_ASSERT(name.IsValid());
	FE_ASSERT(nameString == name.AsString());
	FE_ASSERT(std::hash<fe::Name>()(name) == name.GetHash());

	 //fe::Name invalidName = fe::Name(std::string(reinterpret_cast<const char*>(&L"인코딩")));
	 //fe::Name invalidEmptyName = fe::Name("");
	 //FE_ASSERT(invalidName != invalidEmptyName);

	fe::Pool<unsigned int> pool{ 1, 1 };
	auto				   allocation = pool.Allocate(0);
	auto				   allocation2 = pool.Allocate(1);
	pool.Deallocate(allocation);
	pool.Deallocate(allocation2);

	std::unique_ptr<fe::HandleManager> handleManager = std::make_unique<fe::HandleManager>();
	fe::OwnedHandle<int>			   handle0 = fe::OwnedHandle<int>::Create(*handleManager, "Handle0", 235);
	FE_ASSERT(handle0.IsValid());
	FE_ASSERT(handle0.QueryName() == fe::Name("Handle0"));
	FE_ASSERT(*handle0 == 235);
	FE_ASSERT(handle0);
	fe::WeakHandle<int> weakHandle0 = handle0.DeriveWeak();
	FE_ASSERT(weakHandle0.IsValid());
	FE_ASSERT(weakHandle0.QueryName() == fe::Name("Handle0"));
	FE_ASSERT(*weakHandle0 == 235);
	FE_ASSERT(weakHandle0);

	FE_ASSERT(*weakHandle0 == *handle0);
	FE_ASSERT(weakHandle0 == handle0);
	
	handle0.Destroy();
	FE_ASSERT(!handle0);
	FE_ASSERT(!weakHandle0);

	FE_ASSERT(!handle0.QueryName());
	FE_ASSERT(!weakHandle0.QueryName());

	return 0;
}