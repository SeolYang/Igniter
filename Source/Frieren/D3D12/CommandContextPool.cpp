#include <D3D12/CommandContextPool.h>
#include <D3D12/Device.h>
#include <D3D12/CommandContext.h>
#include <Core/FrameManager.h>
#include <Core/FrameResourceManager.h>

namespace fe::dx
{
	constexpr size_t NumExtraCommandContextsPerFrame = 4;
	CommandContextPool::CommandContextPool(dx::Device& device, const dx::EQueueType queueType)
		: reservedNumCmdCtxs(std::thread::hardware_concurrency() * NumFramesInFlight + (NumExtraCommandContextsPerFrame * NumFramesInFlight))
	{
		check(reservedNumCmdCtxs > 0);
		for (size_t idx = 0; idx < reservedNumCmdCtxs; ++idx)
		{
			pool.push(new dx::CommandContext(device.CreateCommandContext(queueType).value()));
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

	FrameResource<CommandContext> CommandContextPool::Request(FrameResourceManager& frameResourceManager)
	{
		CommandContext* cmdCtxPtr = nullptr;
		if (!pool.try_pop(cmdCtxPtr))
		{
			return {};
		}

		return { cmdCtxPtr,
				 [&frameResourceManager, this](CommandContext* ptr) { frameResourceManager.RequestDeallocation([ptr, this]() { this->Return(ptr); }); } };
	}

	void CommandContextPool::Return(CommandContext* cmdContext)
	{
		check(cmdContext != nullptr);
		check(pool.unsafe_size() < reservedNumCmdCtxs);
		pool.push(cmdContext);
	}
} // namespace fe::dx
