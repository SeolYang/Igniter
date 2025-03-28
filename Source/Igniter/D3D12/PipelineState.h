#pragma once
#include "Igniter/D3D12/Common.h"

namespace ig
{
    class GpuDevice;
    // class PSOCache;
    //  PipelineStateDesc -> private static 캐시를 이용한 flyweight 패턴 구현
    //  buffer, texture 는 내부 데이터가 다를 수 있기 때문에 flyweight 불가능
    class PipelineState final
    {
        friend class GpuDevice;

    public:
        ~PipelineState();

        auto& GetNative() { return *native.Get(); }

        bool IsGraphics() const { return native && bIsGraphics; }
        bool IsCompute() const { return native && !bIsGraphics; }

    private:
        PipelineState(ComPtr<ID3D12PipelineState> newPSO, const bool bIsGraphicsPSO);

    private:
        ComPtr<ID3D12PipelineState> native;
        const bool bIsGraphics;
        // const Size pipelineStateHash;
    };
} // namespace ig
