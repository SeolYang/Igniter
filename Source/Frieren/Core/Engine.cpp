#include <Core/Engine.h>
#include <Core/Assert.h>
#include <Core/Logger.h>
#include <Core/HandleManager.h>

namespace fe
{
	Engine* Engine::instance = nullptr;

	Engine::Engine()
	{
		const bool bIsFirstEngineCreation = instance == nullptr;
		FE_ASSERT(bIsFirstEngineCreation, "Engine instance exist.");
		if (bIsFirstEngineCreation)
		{
			instance = this;

			logger = std::make_unique<Logger>();
			handleManager = std::make_unique<HandleManager>();
		}
	}

	Engine::~Engine()
	{
		if (instance == this)
		{
			instance = nullptr;
		}

		handleManager.reset();
		logger.reset();
	}

	Logger& Engine::GetLogger()
	{
		FE_ASSERT(instance != nullptr, "Engine does not intialized.");
		return *(instance->logger);
	}

	HandleManager& Engine::GetHandleManager()
	{
		FE_ASSERT(instance != nullptr, "Engine does not intialized.");
		return *(instance->handleManager);
	}
} // namespace fe
