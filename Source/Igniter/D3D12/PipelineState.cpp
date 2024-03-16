#include <D3D12/PipelineState.h>
#include <D3D12/RenderDevice.h>
#include <D3D12/PipelineStateDesc.h>

namespace ig
{
    PipelineState::PipelineState(ComPtr<ID3D12PipelineState> newPSO, const bool bIsGraphicsPSO)
        : native(std::move(newPSO)),
          bIsGraphics(bIsGraphicsPSO)
    {
    }

    PipelineState::~PipelineState() {}
} // namespace ig