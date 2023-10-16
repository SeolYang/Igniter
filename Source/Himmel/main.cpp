#include <iostream>
#include <Core/String.h>
#include <Core/Hash.h>
#include <Core/Assert.h>
#include <Core/Name.h>
#include <Core/Pool.h>
#include <Core/HandleManager.h>
#include <Core/Engine.h>
#include <Core/Logger.h>

FE_DECLARE_LOG_CATEGORY(TestCategory, fe::ELogVerbosiy::Fatal);

int main()
{
	fe::Engine engine;
	FE_LOG(TestCategory, "Test");
	int a = 3;
	FE_LOG(fe::LogTemp, "This is temp {}", a);
	FE_LOG(fe::LogInfo, "This is info {}", "info");
	FE_LOG(fe::LogWarn, "This is warning {}", 3.4f);
	FE_LOG(fe::LogError, "Error!");
	FE_LOG(fe::LogFatal, "This is fatal {}", 333333);
	FE_LOG(fe::LogDebug, "This is debug {}, {}", "debug", 3.1415f);
	return 0;
}