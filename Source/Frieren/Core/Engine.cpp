#include <Core/Engine.h>
#include <Core/Assert.h>
#include <Core/Timer.h>
#include <Core/Log.h>
#include <Core/HandleManager.h>
#include <Core/Window.h>
#include <Core/EmbededSettings.h>

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

			timer = std::make_unique<Timer>();
			logger = std::make_unique<Logger>();
			handleManager = std::make_unique<HandleManager>();

			/* @test temp window descriptor */
			const WindowDescription windowDesc{
				.Width = 1280,
				.Height = 720,
				.Title = String(settings::GameName)
			};

			window = std::make_unique<Window>(windowDesc);
		}
	}

	Engine::~Engine()
	{
		if (instance == this)
		{
			instance = nullptr;
		}

		window.reset();
		handleManager.reset();
		logger.reset();
		timer.reset();
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

	int Engine::Execute()
	{
		while (!bShouldExit)
		{
			timer->Begin();

			MSG msg;
			ZeroMemory(&msg, sizeof(msg));
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT)
				{
					bShouldExit = true;
				}

				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			timer->End();
		}

		return 0;
	}

} // namespace fe
