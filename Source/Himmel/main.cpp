#include <iostream>
#include <Core/String.h>
#include <Core/Hash.h>
#include <Core/Assert.h>
#include <Core/Name.h>
#include <Core/Pool.h>
#include <Core/HandleManager.h>
#include <Core/Engine.h>

int main()
{
	fe::Engine engine;
	fe::Engine::GetHandleManager();
	return 0;
}