#pragma once
#include <D3D12/Common.h>
#include <Core/FrameResource.h>

namespace fe::dx
{
	class Device;
	class CommandContext;
	class CommandContextPool
	{
	public:
		CommandContextPool(dx::Device& device, const dx::EQueueType queueType);
		~CommandContextPool();

		FrameResource<dx::CommandContext> Request(FrameResourceManager& frameResourceManager);

	private:
		void Return(dx::CommandContext* cmdContext);

	private:
		const size_t reservedNumCmdCtxs;
		concurrency::concurrent_queue<dx::CommandContext*> pool;
	};
} // namespace fe