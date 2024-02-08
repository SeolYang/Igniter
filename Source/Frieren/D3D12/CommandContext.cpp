#include <D3D12/CommandContext.h>
#include <D3D12/Device.h>
#include <D3D12/PipelineState.h>

namespace fe::dx
{
	CommandContext::CommandContext(Device& device, const String /*debugName*/, const D3D12_COMMAND_LIST_TYPE type)
		: typeOfCommandList(type)
	{
		auto& nativeDevice = device.GetNative();
		verify_succeeded(
			nativeDevice.CreateCommandList1(0, type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&cmdList)));
		verify_succeeded(nativeDevice.CreateCommandAllocator(type, IID_PPV_ARGS(&cmdAllocator)));
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
} // namespace fe::dx