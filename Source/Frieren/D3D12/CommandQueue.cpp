#include <D3D12/CommandQueue.h>
#include <D3D12/CommandContext.h>
#include <D3D12/Device.h>
#include <D3D12/Fence.h>
#include <Core/Utility.h>

namespace fe::dx
{
	CommandQueue::CommandQueue(ComPtr<ID3D12CommandQueue> newNativeQueue, const EQueueType specifiedType)
		: native(std::move(newNativeQueue)), type(specifiedType)
	{
	}

	CommandQueue::CommandQueue(CommandQueue&& other) noexcept
		: native(std::move(other.native)), type(other.type), pendingContexts(std::move(other.pendingContexts))
	{
		pendingContexts.reserve(RecommendedMinNumCommandContexts);
	}

	CommandQueue::~CommandQueue() {}

	void CommandQueue::SignalTo(Fence& fence)
	{
		check(IsValid());
		check(fence);
		verify_succeeded(native->Signal(&fence.GetNative(), fence.GetCounter()));
	}

	void CommandQueue::NextSignalTo(Fence& fence)
	{
		check(IsValid());
		check(fence);
		fence.Next();
		SignalTo(fence);
	}

	void CommandQueue::WaitFor(Fence& fence)
	{
		check(IsValid());
		check(fence);
		verify_succeeded(native->Wait(&fence.GetNative(), fence.GetCounter()));
	}

	void CommandQueue::AddPendingContext(CommandContext& cmdCtx)
	{
		check(cmdCtx);
		// #todo Assertion on Nvidia do and dont's limitations
		// ifdef FE_ASSERT_ON_NVIDIA_DO_AND_DONTS_LIMITS
		// check(pendingContexts.size() < RecommendedMaxNumCommandContexts);
		pendingContexts.emplace_back(cmdCtx);
	}

	void CommandQueue::FlushPendingContexts()
	{
		check(IsValid());
		check(!pendingContexts.empty());
		auto pendingNativeCmdLists = TransformToNative(std::span{ pendingContexts });
		native->ExecuteCommandLists(static_cast<uint32_t>(pendingNativeCmdLists.size()),
									reinterpret_cast<ID3D12CommandList* const*>(pendingNativeCmdLists.data()));
		pendingContexts.clear();
	}

	void CommandQueue::FlushQueue(Device& device) 
	{
		auto fence = device.CreateFence("Tmp");
		check(fence);
		NextSignalTo(*fence);
		fence->WaitOnCPU();
	}
} // namespace fe::dx