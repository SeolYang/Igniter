#pragma once
#include <D3D12/Common.h>
#include <D3D12/CommandContext.h>
#include <Core/Container.h>

namespace fe
{
	class DeferredDeallocator;
	void RequestDeferredDeallocation(DeferredDeallocator& deferredDeallocator, DefaultCallback requester);
}

namespace fe
{
	class RenderDevice;
	class CommandContextPool
	{
	public:
		CommandContextPool(DeferredDeallocator& deferredDeallocator, RenderDevice& device, const EQueueType queueType);
		~CommandContextPool();

		auto Submit(const std::string_view debugName = "")
		{
			const auto deleter = [this](CommandContext* ptr) {
				if (ptr != nullptr)
				{
					RequestDeferredDeallocation(deferredDeallocator, [ptr, this]() { this->Return(ptr); });
				}
			};

			using ReturnType = std::unique_ptr<CommandContext, decltype(deleter)>;

			CommandContext* cmdCtxPtr = nullptr;
			if (!pool.try_pop(cmdCtxPtr))
			{
				return ReturnType{ nullptr, deleter };
			}

			if (!debugName.empty())
			{
				SetObjectName(&cmdCtxPtr->GetNative(), debugName);
			}

			return ReturnType{ cmdCtxPtr, deleter };
		}

	private:
		void Return(CommandContext* cmdContext);

	private:
		DeferredDeallocator& deferredDeallocator;
		const size_t reservedNumCmdCtxs;
		concurrency::concurrent_queue<CommandContext*> pool;
	};
} // namespace fe