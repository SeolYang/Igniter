#include <D3D12/CommandContextPool.h>
#include <D3D12/RenderDevice.h>
#include <D3D12/CommandContext.h>
#include <Core/FrameManager.h>
#include <Core/Assert.h>
#include <Core/DeferredDeallocator.h>

namespace fe
{
	constexpr size_t NumExtraCommandContextsPerFrame = 4;
	CommandContextPool::CommandContextPool(DeferredDeallocator& deferredDeallocator, RenderDevice& device, const EQueueType queueType)
		: deferredDeallocator(deferredDeallocator),
		  reservedNumCmdCtxs(std::thread::hardware_concurrency() * NumFramesInFlight + (NumExtraCommandContextsPerFrame * NumFramesInFlight))
	{
		check(reservedNumCmdCtxs > 0);
		for (size_t idx = 0; idx < reservedNumCmdCtxs; ++idx)
		{
			pool.push(new CommandContext(device.CreateCommandContext("Reserved Cmd Ctx", queueType).value()));
		}
	}

	CommandContextPool::~CommandContextPool()
	{
		check(pool.unsafe_size() == reservedNumCmdCtxs);
		for (auto begin = pool.unsafe_begin(); begin != pool.unsafe_end(); ++begin)
		{
			delete *begin;
		}
		pool.clear();
	}

	void CommandContextPool::Return(CommandContext* cmdContext)
	{
		check(cmdContext != nullptr);
		check(pool.unsafe_size() < reservedNumCmdCtxs);
		pool.push(cmdContext);
	}

} // namespace fe
