#include "Igniter/Igniter.h"
#include "Igniter/D3D12/PipelineState.h"

namespace ig
{
    PipelineState::PipelineState(ComPtr<ID3D12PipelineState> newPSO, const bool bIsGraphicsPSO)
        : native(std::move(newPSO))
        , bIsGraphics(bIsGraphicsPSO)
    {
    }

    PipelineState::~PipelineState() {}
} // namespace ig
