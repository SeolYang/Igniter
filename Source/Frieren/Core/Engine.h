#pragma once
#include <memory>

namespace fe
{
	class HandleManager;
	class Logger;
	class Engine final
	{
	public:
		Engine();
		~Engine();

		[[nodiscard]] static Logger& GetLogger();
		[[nodiscard]] static HandleManager& GetHandleManager();
		bool								IsValid() const { return this == instance; }

	private:
		static Engine*				   instance;
		std::unique_ptr<Logger>		   logger;
		std::unique_ptr<HandleManager> handleManager;
	};
} // namespace fe
