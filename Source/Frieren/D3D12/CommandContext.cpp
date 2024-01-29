#include <D3D12/CommandContext.h>
#include <D3D12/Device.h>
#include <D3D12/PipelineState.h>

namespace fe::dx
{
	CommandContext::CommandContext(const Device& device, const String debugName, const D3D12_COMMAND_LIST_TYPE type)
		: typeOfCommandList(type)
	{
		ID3D12Device10& nativeDevice = device.GetNative();
		verify_succeeded(nativeDevice.CreateCommandList1(0, type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&cmdList)));
		verify_succeeded(nativeDevice.CreateCommandAllocator(type, IID_PPV_ARGS(&cmdAllocator)));
	}

	void CommandContext::Begin(const PipelineState* initState)
	{
		verify(cmdAllocator.Get() != nullptr);
		verify(cmdList.Get() != nullptr);

		ID3D12PipelineState* nativeInitState = initState != nullptr ? &initState->GetNative() : nullptr;
		verify(initState == nullptr || (initState != nullptr && nativeInitState != nullptr));

		verify_succeeded(cmdAllocator->Reset());
		verify_succeeded(cmdList->Reset(cmdAllocator.Get(), nativeInitState));
	}

	void CommandContext::End()
	{
		verify(cmdList.Get() != nullptr);
		verify_succeeded(cmdList->Close());
	}

} // namespace fe