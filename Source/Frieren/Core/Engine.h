#pragma once
#include <memory>

namespace fe
{
	class HandleManager;
	class Engine final
	{
	public:
		Engine();
		~Engine();

		[[nodiscard]] static HandleManager& GetHandleManager();
		bool								IsValid() const { return this == instance; }

	private:
		static Engine*				   instance;
		std::unique_ptr<HandleManager> handleManager;
	};
} // namespace fe
