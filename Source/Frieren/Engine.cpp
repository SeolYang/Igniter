#include <Engine.h>
#include <Core/Assert.h>
#include <Core/Timer.h>
#include <Core/Log.h>
#include <Core/HandleManager.h>
#include <Core/InputManager.h>
#include <Core/Window.h>
#include <Core/EmbededSettings.h>
#include <Renderer/Renderer.h>
#include <Gameplay/GameInstance.h>
#include <Gameplay/World.h>

namespace fe
{
	Engine* Engine::instance = nullptr;

	Engine::Engine()
	{
		const bool bIsFirstEngineCreation = instance == nullptr;
		FE_ASSERT(bIsFirstEngineCreation, "Engine instance already exist.");
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
			inputManager = std::make_unique<InputManager>();

			renderer = std::make_unique<Renderer>();

			gameInstance = std::make_unique<GameInstance>();
		}
	}

	Engine::~Engine()
	{
		gameInstance.reset();
		renderer.reset();
		inputManager.reset();
		window.reset();
		handleManager.reset();
		logger.reset();
		timer.reset();

		if (instance == this)
		{
			instance = nullptr;
		}
	}

	Timer& Engine::GetTimer()
	{
		FE_ASSERT(instance != nullptr, "Engine does not intialized.");
		return *(instance->timer);
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

	Window& Engine::GetWindow()
	{
		FE_ASSERT(instance != nullptr, "Engine does not intialized.");
		return *(instance->window);
	}

	InputManager& Engine::GetInputManager()
	{
		FE_ASSERT(instance != nullptr, "Engine does not intialized.");
		return *(instance->inputManager);
	}

	Renderer& Engine::GetRenderer()
	{
		FE_ASSERT(instance != nullptr, "Engine does not intialized.");
		return *(instance->renderer);
	}

	GameInstance& Engine::GetGameInstance()
	{
		FE_ASSERT(instance != nullptr, "Engine does not intialized.");
		return *(instance->gameInstance);
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

			{
				gameInstance->Update();
			}

			{
				inputManager->PostUpdate();
			}

			timer->End();
		}

		return 0;
	}

} // namespace fe
