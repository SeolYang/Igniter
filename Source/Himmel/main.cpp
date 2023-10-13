#include <iostream>
#include <Core/String.h>
#include <Core/Hash.h>
#include <Core/Assert.h>
#include <Core/Name.h>
#include <Core/Pool.h>
#include <Core/HandleManager.h>

int main()
{
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