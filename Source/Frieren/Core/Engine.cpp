#include <Core/Engine.h>
#include <Core/Assert.h>
#include <Core/Timer.h>
#include <Core/Log.h>
#include <Core/HandleManager.h>
#include <Core/InputManager.h>
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
			inputManager = std::make_unique<InputManager>();
			/* @test */
			inputManager->BindAction(String("Jump"), EInput::Space);
			inputManager->BindAction(String("Forward"), EInput::W);
			inputManager->BindAction(String("Left"), EInput::A);
			inputManager->BindAction(String("Backward"), EInput::S);
			inputManager->BindAction(String("Right"), EInput::D);
			inputManager->BindAxis(String("MoveForward"), EInput::W, 1.f);
			inputManager->BindAxis(String("MoveForward"), EInput::S, -1.f);
			inputManager->BindAxis(String("MoveLeft"), EInput::A, 1.f);
			inputManager->BindAxis(String("MoveLeft"), EInput::D, -1.f);
			inputManager->BindAxis(String("MouseX"), EInput::MouseX, 1.f);
			inputManager->BindAxis(String("MouseY"), EInput::MouseY, 1.f);
			inputManager->BindAxis(String("RelativeMouseX"), EInput::RelativeMouseX, 1.f);
			inputManager->BindAxis(String("RelativeMouseY"), EInput::RelativeMouseY, 1.f);

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

		inputManager.reset();
		handleManager.reset();
		window.reset();
		logger.reset();
		timer.reset();
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

	fe::Window& Engine::GetWindow()
	{
		FE_ASSERT(instance != nullptr, "Engine does not intialized.");
		return *(instance->window);
	}

	InputManager& Engine::GetInputManager()
	{
		FE_ASSERT(instance != nullptr, "Engine does not intialized.");
		return *(instance->inputManager);
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
