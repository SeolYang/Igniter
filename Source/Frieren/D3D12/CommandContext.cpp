#include <D3D12/CommandContext.h>
#include <D3D12/Device.h>
#include <D3D12/PipelineState.h>

namespace fe::dx
{
	CommandContext::CommandContext(const Device& device, const String debugName, const D3D12_COMMAND_LIST_TYPE type)
		: typeOfCommandList(type)
	{
		ID3D12Device10& nativeDevice = device.GetNative();
		FE_SUCCEEDED_ASSERT(nativeDevice.CreateCommandList1(0, type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&cmdList)));
		FE_SUCCEEDED_ASSERT(nativeDevice.CreateCommandAllocator(type, IID_PPV_ARGS(&cmdAllocator)));
	}

	void CommandContext::Begin(const PipelineState* initState)
	{
		FE_ASSERT(cmdAllocator.Get() != nullptr);
		FE_ASSERT(cmdList.Get() != nullptr);

		ID3D12PipelineState* nativeInitState = initState != nullptr ? &initState->GetNative() : nullptr;
		FE_ASSERT(initState == nullptr || (initState != nullptr && nativeInitState != nullptr));

		FE_SUCCEEDED_ASSERT(cmdAllocator->Reset());
		FE_SUCCEEDED_ASSERT(cmdList->Reset(cmdAllocator.Get(), nativeInitState));
	}

	void CommandContext::End()
	{
		FE_ASSERT(cmdList.Get() != nullptr);
		FE_SUCCEEDED_ASSERT(cmdList->Close());
	}

} // namespace fe