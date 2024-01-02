#pragma once
#include <memory>
#include <Core/Version.h>

namespace fe
{
	constexpr Version EngineVersion = CreateVersion(0, 2, 1);

	class Timer;
	class Logger;
	class HandleManager;
	class Window;
	class InputManager;
	class Renderer;
	class GameInstance;
	class Engine final
	{
	public:
		Engine();
		~Engine();

		[[nodiscard]] static Timer&			GetTimer();
		[[nodiscard]] static Logger&		GetLogger();
		[[nodiscard]] static HandleManager& GetHandleManager();
		[[nodiscard]] static Window&		GetWindow();
		[[nodiscard]] static InputManager&	GetInputManager();
		[[nodiscard]] static Renderer&		GetRenderer();
		[[nodiscard]] static GameInstance&	GetGameInstance();
		bool								IsValid() const { return this == instance; }

		int Execute();

	private:
		static Engine*				   instance;
		bool						   bShouldExit = false;
		std::unique_ptr<Timer>		   timer;
		std::unique_ptr<Logger>		   logger;
		std::unique_ptr<HandleManager> handleManager;
		std::unique_ptr<Window>		   window;
		std::unique_ptr<InputManager>  inputManager;
		std::unique_ptr<Renderer>	   renderer;
		std::unique_ptr<GameInstance>  gameInstance;
	};
} // namespace fe
