#pragma once
#include <Renderer/Common.h>

namespace fe::dx
{
	class CommandContext;
}

namespace fe
{
	struct CommandContextPackage
	{
		friend class CommandContextPool;

	public:
		~CommandContextPackage() = default;

		bool IsValid() const
		{
			return commandContext != nullptr;
		}

		operator bool() const
		{
			return IsValid();
		}

	private:
		CommandContextPackage(dx::CommandContext& commandContext, const uint8_t currentLocalFrameIdx);

	private:
		dx::CommandContext* commandContext;
		const uint8_t		ownedLocalFrameIdx;
	};
	using CommandContextPackagePtr = std::unique_ptr<CommandContextPackage, void (*)(CommandContextPackage*)>;

	class CommandContextPool
	{
	public:
		CommandContextPackagePtr Request(const uint8_t currentLocalFrameIdx);

	private:
		// cmd contexts/thread
		std::vector<std::queue<dx::CommandContext*>>
	};
} // namespace fe