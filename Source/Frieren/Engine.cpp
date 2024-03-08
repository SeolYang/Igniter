#include <Engine.h>
#include <Core/Assert.h>
#include <Core/Log.h>
#include <Core/HandleManager.h>
#include <Core/InputManager.h>
#include <Core/Window.h>
#include <Core/DeferredDeallocator.h>
#include <Core/EmbededSettings.h>
#include <D3D12/RenderDevice.h>
#include <Render/GpuUploader.h>
#include <Render/GPUViewManager.h>
#include <Render/Renderer.h>
#include <ImGui/ImGuiRenderer.h>
#include <ImGui/ImGuiCanvas.h>
#include <Gameplay/GameInstance.h>
#include <Gameplay/World.h>

namespace fe
{
	FE_DEFINE_LOG_CATEGORY(EngineInfo, ELogVerbosiy::Info)
	FE_DEFINE_LOG_CATEGORY(EngineFatal, ELogVerbosiy::Fatal)

	Engine* Engine::instance = nullptr;
	Engine::Engine()
	{
		const bool bIsFirstEngineCreation = instance == nullptr;
		check(bIsFirstEngineCreation);
		if (bIsFirstEngineCreation)
		{
			instance = this;
			logger = std::make_unique<Logger>();
			logger->Log<EngineInfo>("Engine version: {}", version::Version);
			logger->Log<EngineInfo>("Initializing Engine Runtime...");

			/* #test 임시 윈도우 설명자 */
			const WindowDescription winDesc{ .Width = 1280, .Height = 720, .Title = String(settings::GameName) };
			window = std::make_unique<Window>(winDesc);
			renderDevice = std::make_unique<RenderDevice>();
			gpuUploader = std::make_unique<GpuUploader>(*renderDevice);
			handleManager = std::make_unique<HandleManager>();
			deferredDeallocator = std::make_unique<DeferredDeallocator>(frameManager);
			inputManager = std::make_unique<InputManager>(*handleManager);
			gpuViewManager = std::make_unique<GpuViewManager>(*handleManager, *deferredDeallocator, *renderDevice);
			renderer = std::make_unique<Renderer>(frameManager, *deferredDeallocator, *window, *renderDevice, *handleManager, *gpuViewManager);
			imguiRenderer = std::make_unique<ImGuiRenderer>(frameManager, *window, *renderDevice);
			imguiCanvas = std::make_unique<ImGuiCanvas>();
			gameInstance = std::make_unique<GameInstance>();
		}
	}

	Engine::~Engine()
	{
		logger->Log<EngineInfo>("* Cleanup sub-systems");
		deferredDeallocator->FlushAllFrames();
		gameInstance.reset();
		imguiCanvas.reset();
		imguiRenderer.reset();
		renderer.reset();
		inputManager.reset();
		deferredDeallocator.reset();
		handleManager.reset();
		gpuViewManager.reset();
		gpuUploader.reset();
		renderDevice.reset();
		window.reset();
		logger.reset();

		if (instance == this)
		{
			instance = nullptr;
		}

		String::ClearCache();
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
		return *instance->logger;
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

	RenderDevice& Engine::GetRenderDevice()
	{
		check(instance != nullptr);
		return *(instance->renderDevice);
	}

	GpuUploader& Engine::GetGpuUploader()
	{
		check(instance != nullptr);
		return *(instance->gpuUploader);
	}

	InputManager& Engine::GetInputManager()
	{
		check(instance != nullptr);
		return *(instance->inputManager);
	}

	GpuViewManager& Engine::GetGPUViewManager()
	{
		check(instance != nullptr);
		return *(instance->gpuViewManager);
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
		logger->Log<EngineInfo>("* Start Engine main loop");

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

			deferredDeallocator->FlushCurrentFrame();

			gameInstance->Update();
			inputManager->PostUpdate();

			renderer->BeginFrame();

			if (gameInstance->HasWorld())
			{
				renderer->Render(gameInstance->GetWorld());
			}
			imguiRenderer->Render(*imguiCanvas, *renderer);

			renderer->EndFrame();

			timer.End();
			frameManager.NextFrame();
		}

		renderer->FlushQueues();
		logger->Log<EngineInfo>("* End Engine main loop");
		return 0;
	}
} // namespace fe
