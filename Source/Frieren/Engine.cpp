#include <Engine.h>
#include <Core/Assert.h>
#include <Core/Log.h>
#include <Core/HandleManager.h>
#include <Core/InputManager.h>
#include <Core/Window.h>
#include <Core/FrameResourceManager.h>
#include <Core/EmbededSettings.h>
#include <D3D12/Device.h>
#include <Renderer/Renderer.h>
#include <ImGui/ImGuiRenderer.h>
#include <ImGui/ImGuiCanvas.h>
#include <Gameplay/GameInstance.h>
#include <Gameplay/World.h>

namespace fe
{
	FE_DECLARE_LOG_CATEGORY(EngineInfo, ELogVerbosiy::Info)

	Engine* Engine::instance = nullptr;
	Engine::Engine()
	{
		const bool bIsFirstEngineCreation = instance == nullptr;
		check(bIsFirstEngineCreation);
		if (bIsFirstEngineCreation)
		{
			logger.Log<EngineInfo>("Initialize Engine Runtime...");
			logger.Log<EngineInfo>("Engine version: {}", version::Version);

			instance = this;

			handleManager = std::make_unique<HandleManager>();

			/* @test temp window descriptor */
			const WindowDescription windowDesc{ .Width = 1280, .Height = 720, .Title = String(settings::GameName) };

			window = std::make_unique<Window>(windowDesc);
			inputManager = std::make_unique<InputManager>();

			frameResourceManager = std::make_unique<FrameResourceManager>(frameManager);

			renderer = std::make_unique<Renderer>(frameManager, *window);

			imguiRenderer = std::make_unique<ImGuiRenderer>(frameManager, renderer->GetDevice(), *window);
			imguiCanvas = std::make_unique<ImGuiCanvas>();

			gameInstance = std::make_unique<GameInstance>();

			logger.Log<EngineInfo>("Engine Runtime Initialized.");
		}
	}

	Engine::~Engine()
	{
		logger.Log<EngineInfo>("De-initialize Engine Runtime...");

		frameResourceManager.reset();

		gameInstance.reset();
		imguiCanvas.reset();
		imguiRenderer.reset();
		renderer.reset();
		inputManager.reset();
		window.reset();
		handleManager.reset();

		if (instance == this)
		{
			instance = nullptr;
		}
		logger.Log<EngineInfo>("Engine Runtime De-initialized.");
	}

	FrameManager& Engine::GetFrameManager()
	{
		check(instance != nullptr);
		return instance->frameManager;
	}

	Timer& Engine::GetTimer()
	{
		check(instance != nullptr);
		return instance->timer;
	}

	Logger& Engine::GetLogger()
	{
		check(instance != nullptr);
		return instance->logger;
	}

	HandleManager& Engine::GetHandleManager()
	{
		check(instance != nullptr);
		return *(instance->handleManager);
	}

	Window& Engine::GetWindow()
	{
		check(instance != nullptr);
		return *(instance->window);
	}

	InputManager& Engine::GetInputManager()
	{
		check(instance != nullptr);
		return *(instance->inputManager);
	}

	Renderer& Engine::GetRenderer()
	{
		check(instance != nullptr);
		return *(instance->renderer);
	}

	ImGuiRenderer& Engine::GetImGuiRenderer()
	{
		check(instance != nullptr);
		return *(instance->imguiRenderer);
	}

	ImGuiCanvas& Engine::GetImGuiCanvas()
	{
		check(instance != nullptr);
		return *(instance->imguiCanvas);
	}

	GameInstance& Engine::GetGameInstance()
	{
		check(instance != nullptr);
		return *(instance->gameInstance);
	}

	int Engine::Execute()
	{
		logger.Log<EngineInfo>("Begin => Engine Main Loop");

		while (!bShouldExit)
		{
			timer.Begin();

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

			gameInstance->Update();
			inputManager->PostUpdate();

			renderer->BeginFrame();
			frameResourceManager->BeginFrame();

			renderer->Render();
			imguiRenderer->Render(*imguiCanvas, *renderer);

			renderer->EndFrame();

			timer.End();
			frameManager.NextFrame();
		}

		logger.Log<EngineInfo>("End => Engine Main Loop");
		return 0;
	}

} // namespace fe
