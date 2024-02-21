#pragma once
#include <D3D12/Common.h>
#include <Core/Container.h>

namespace fe
{
	class DeferredDeallocator;
}

namespace fe::Private
{
	void RequestDeallocation(DeferredDeallocator& deferredDeallocator, DefaultCallback&& requester);
}

namespace fe::dx
{
	class Device;
	class CommandContext;
	class CommandContextPool
	{
	public:
		CommandContextPool(DeferredDeallocator& deferredDeallocator, dx::Device& device, const dx::EQueueType queueType);
		~CommandContextPool();

		auto Request()
		{
			const auto lambda = [this](CommandContext* ptr) {
				if (ptr != nullptr)
				{
					Private::RequestDeallocation(deferredDeallocator, [ptr, this]() { this->Return(ptr); });
				}
			};

			using return_t = std::unique_ptr<CommandContext, decltype(lambda)>;

			CommandContext* cmdCtxPtr = nullptr;
			if (!pool.try_pop(cmdCtxPtr))
			{
				return return_t{ nullptr, lambda };
			}

			return return_t{ cmdCtxPtr, lambda };
		}

	private:
		void Return(dx::CommandContext* cmdContext);

	private:
		DeferredDeallocator& deferredDeallocator;
		const size_t reservedNumCmdCtxs;
		concurrency::concurrent_queue<dx::CommandContext*> pool;
	};
} // namespace fe