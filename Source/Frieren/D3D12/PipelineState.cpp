#include <D3D12/PipelineState.h>
#include <D3D12/Device.h>
#include <D3D12/PipelineStateDesc.h>

namespace fe::dx
{
	PipelineState::PipelineState(ComPtr<ID3D12PipelineState> newPSO, const bool bIsGraphicsPSO)
		: native(std::move(newPSO)), bIsGraphics(bIsGraphicsPSO)
	{
	}

	PipelineState::~PipelineState() {}
} // namespace fe::dx