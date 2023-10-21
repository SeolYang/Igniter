#pragma once
#include <memory>
#include <Core/Version.h>

namespace fe
{
	constexpr Version EngineVersion = CreateVersion(0, 1, 0);

	class HandleManager;
	class Logger;
	class Window;
	class Timer;
	class InputManager;
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
		bool								IsValid() const { return this == instance; }

		int Execute();

	private:
		static Engine*				   instance;
		bool						   bShouldExit = false;
		std::unique_ptr<Timer>		   timer;
		std::unique_ptr<Logger>		   logger;
		std::unique_ptr<Window>		   window;
		std::unique_ptr<HandleManager> handleManager;
		std::unique_ptr<InputManager>  inputManager;
	};
} // namespace fe
