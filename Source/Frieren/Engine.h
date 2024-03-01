#pragma once
#include <memory>
#include <Core/Version.h>
#include <Core/FrameManager.h>
#include <Core/Timer.h>

namespace fe
{
	class Timer;
	class Logger;
	class HandleManager;
	class Window;
	class InputManager;
	class RenderDevice;
	class GpuUploader;
	class GpuViewManager;
	class DeferredDeallocator;
	class Renderer;
	class GameInstance;
	class ImGuiCanvas;
	class ImGuiRenderer;
	class Engine final
	{
	public:
		Engine();
		~Engine();

		[[nodiscard]] static FrameManager& GetFrameManager();
		[[nodiscard]] static Timer& GetTimer();
		[[nodiscard]] static Logger& GetLogger();
		[[nodiscard]] static HandleManager& GetHandleManager();
		[[nodiscard]] static Window& GetWindow();
		[[nodiscard]] static RenderDevice& GetRenderDevice();
		[[nodiscard]] static GpuUploader& GetGpuUploader();
		[[nodiscard]] static InputManager& GetInputManager();
		[[nodiscard]] static GpuViewManager& GetGPUViewManager();
		[[nodiscard]] static Renderer& GetRenderer();
		[[nodiscard]] static ImGuiRenderer& GetImGuiRenderer();
		[[nodiscard]] static ImGuiCanvas& GetImGuiCanvas();
		[[nodiscard]] static GameInstance& GetGameInstance();
		bool IsValid() const { return this == instance; }

		int Execute();

	private:
		static Engine* instance;
		bool bShouldExit = false;
		FrameManager frameManager;
		Timer timer;
		std::unique_ptr<Logger> logger;
		std::unique_ptr<Window> window;
		std::unique_ptr<RenderDevice> renderDevice;
		std::unique_ptr<GpuUploader> gpuUploader;
		std::unique_ptr<HandleManager> handleManager;
		std::unique_ptr<DeferredDeallocator> deferredDeallocator;
		std::unique_ptr<InputManager> inputManager;
		std::unique_ptr<GpuViewManager> gpuViewManager;
		std::unique_ptr<Renderer> renderer;
		std::unique_ptr<ImGuiRenderer> imguiRenderer;
		std::unique_ptr<ImGuiCanvas> imguiCanvas;
		std::unique_ptr<GameInstance> gameInstance;
	};
} // namespace fe
