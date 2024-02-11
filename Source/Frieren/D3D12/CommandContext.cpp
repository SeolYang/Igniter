#include <D3D12/CommandContext.h>
#include <D3D12/Device.h>
#include <D3D12/PipelineState.h>

namespace fe::dx
{
	CommandContext::CommandContext(CommandContext&& other) noexcept
		: cmdAllocator(std::move(other.cmdAllocator))
		, cmdList(std::move(other.cmdList))
		, cmdListTargetQueueType(other.cmdListTargetQueueType)
		, pendingGlobalBarriers(std::move(other.pendingGlobalBarriers))
		, pendingTextureBarriers(std::move(other.pendingTextureBarriers))
		, pendingBufferBarriers(std::move(other.pendingBufferBarriers))
	{
	}

	CommandContext::CommandContext(ComPtr<ID3D12CommandAllocator>	  newCmdAllocator,
								   ComPtr<ID3D12GraphicsCommandList7> newCmdList,
								   const D3D12_COMMAND_LIST_TYPE	  targetQueueType)
		: cmdAllocator(std::move(newCmdAllocator))
		, cmdList(std::move(newCmdList))
		, cmdListTargetQueueType(targetQueueType)
	{
	}

	void CommandContext::Begin(PipelineState* const initStatePtr)
	{
		verify(cmdAllocator.Get() != nullptr);
		verify(cmdList.Get() != nullptr);

		auto* const nativeInitStatePtr = initStatePtr != nullptr ? &initStatePtr->GetNative() : nullptr;
		verify(initStatePtr == nullptr || (initStatePtr != nullptr && nativeInitStatePtr != nullptr));
		verify_succeeded(cmdAllocator->Reset());
		verify_succeeded(cmdList->Reset(cmdAllocator.Get(), nativeInitStatePtr));
	}

	void CommandContext::End()
	{
		verify(cmdList.Get() != nullptr);
		verify_succeeded(cmdList->Close());
	}

	CommandContext& CommandContext::operator=(CommandContext&& other) noexcept
	{
		cmdAllocator = std::move(other.cmdAllocator);
		cmdList = std::move(other.cmdList);
		cmdListTargetQueueType = other.cmdListTargetQueueType;
		pendingGlobalBarriers = std::move(other.pendingGlobalBarriers);
		pendingTextureBarriers = std::move(other.pendingTextureBarriers);
		pendingBufferBarriers = std::move(other.pendingBufferBarriers);
		return *this;
	}

} // namespace fe::dx