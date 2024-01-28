#include <D3D12/PipelineState.h>
#include <D3D12/Device.h>
#include <D3D12/PipelineStateDesc.h>

namespace fe::dx
{
	PipelineState::PipelineState(const Device& device, const GraphicsPipelineStateDesc& desc)
		: bIsGraphicsPSO(true), name(desc.Name)
	{
		ID3D12Device10& nativeDevice = device.GetNative();
		FE_SUCCEEDED_ASSERT(nativeDevice.CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso)));

		// #todo PSOCache!
		const std::wstring widerName = name.AsWideString();
		pso->SetName(widerName.c_str());
	}

	PipelineState::PipelineState(const Device& device, const ComputePipelineStateDesc& desc)
		: bIsGraphicsPSO(false), name(desc.Name)
	{
		ID3D12Device10& nativeDevice = device.GetNative();
		FE_SUCCEEDED_ASSERT(nativeDevice.CreateComputePipelineState(&desc, IID_PPV_ARGS(&pso)));

		// #todo PSOCache!
		const std::wstring widerName = name.AsWideString();
		pso->SetName(widerName.c_str());
	}

	PipelineState::~PipelineState()
	{
	}
} // namespace fe