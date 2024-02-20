#include <Engine.h>
#include <Core/Assert.h>
#include <Core/Log.h>
#include <Core/HandleManager.h>
#include <Core/InputManager.h>
#include <Core/Window.h>
#include <Core/DeferredDeallocator.h>
#include <Core/EmbededSettings.h>
#include <D3D12/Device.h>
#include <Renderer/Renderer.h>
#include <ImGui/ImGuiRenderer.h>
#include <ImGui/ImGuiCanvas.h>
#include <Gameplay/GameInstance.h>
#include <Gameplay/World.h>

namespace fe
{
	FE_DECLARE_LOG_CATEGORY(TestInfo, ELogVerbosiy::Info)

	class FrameHandleTestClass
	{
	public:
		FrameHandleTestClass(const size_t value)
			: data(value)
		{
			FE_LOG(TestInfo, "Test Constructor! {}", data);
		}

		~FrameHandleTestClass()
		{
			// estimate: Test Destructor! globalFrameIdx:2
			FE_LOG(TestInfo, "Test Destructor! globalFrameIdx:{}", Engine::GetFrameManager().GetGlobalFrameIndex());
		}

	public:
		uint64_t data = 0;
	};
} // namespace fe

namespace fe
{
	FE_DECLARE_LOG_CATEGORY(EngineInfo, ELogVerbosiy::Info)
	FE_DECLARE_LOG_CATEGORY(EngineFatal, ELogVerbosiy::Fatal)

	Engine* Engine::instance = nullptr;
	Engine::Engine()
	{
		const bool bIsFirstEngineCreation = instance == nullptr;
		check(bIsFirstEngineCreation);
		if (bIsFirstEngineCreation)
		{
			logger.Log<EngineInfo>("Engine version: {}", version::Version);
			logger.Log<EngineInfo>("Initializing Engine Runtime...");

			instance = this;
			/* #test temp window descriptor */
			const WindowDescription windowDesc{ .Width = 1280, .Height = 720, .Title = String(settings::GameName) };
			window = std::make_unique<Window>(windowDesc);
			renderDevice = std::make_unique<dx::Device>();
			handleManager = std::make_unique<HandleManager>();
			deferredDeallocator = std::make_unique<DeferredDeallocator>(frameManager);
			inputManager = std::make_unique<InputManager>(*handleManager);
			renderer = std::make_unique<Renderer>(frameManager, *deferredDeallocator, *window, *renderDevice);
			imguiRenderer = std::make_unique<ImGuiRenderer>(frameManager, *window, *renderDevice);
			imguiCanvas = std::make_unique<ImGuiCanvas>();
			gameInstance = std::make_unique<GameInstance>();

			// #test FrameHandle 테스트
			check(!frameHandle.IsAlive());
			frameHandle = FrameHandle<FrameHandleTestClass>{ *handleManager, *deferredDeallocator, 1303 };
			check(frameHandle.IsAlive());
			check(frameHandle->data == 1303);
		}
	}

	Engine::~Engine()
	{
		logger.Log<EngineInfo>("* Cleanup sub-systems");

		gameInstance.reset();
		imguiCanvas.reset();
		imguiRenderer.reset();
		renderer.reset();
		inputManager.reset();
		deferredDeallocator.reset();
		handleManager.reset();
		renderDevice.reset();
		window.reset();

		if (instance == this)
		{
			instance = nullptr;
		}
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
		logger.Log<EngineInfo>("* Start Engine main loop");

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

			// #test FrameHandle 테스트
			if (frameManager.GetGlobalFrameIndex() == 0)
			{
				check(frameHandle.IsAlive());
				check(frameHandle->data == 1303);

				WeakHandle<FrameHandleTestClass> weakHandle = frameHandle.DeriveWeak();
				check(weakHandle.IsAlive());
				check(weakHandle->data == 1303);

				frameHandle.Destroy();
				check(!frameHandle.IsAlive());
				// Destroy후 같은 값을 가지는 Handle이 재 할당되면, 남아있는 weak handle 들의
				// 유효성은? ; 해결 방법 -> versioning => 추가 데이터 필요 -> 헤더 추가? -> 추가 메모리 비용 발생 예상
				// -> Alignment도 문제.
				check(!weakHandle.IsAlive());
				// weakHandle->data = 1234; // fail
			}

			renderer->BeginFrame();

			renderer->Render();
			imguiRenderer->Render(*imguiCanvas, *renderer);

			renderer->EndFrame();

			timer.End();
			frameManager.NextFrame();
		}

		renderer->WaitForFences();
		deferredDeallocator->FlushAllFrames();
		logger.Log<EngineInfo>("* End Engine main loop");
		return 0;
	}
} // namespace fe
