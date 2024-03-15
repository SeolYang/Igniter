#pragma once
#include <D3D12/Common.h>
#include <Core/String.h>
#include <span>

namespace fe
{
    class ShaderBlob;
    class RootSignature;
    class GraphicsPipelineStateDesc : public D3D12_GRAPHICS_PIPELINE_STATE_DESC
    {
    public:
        GraphicsPipelineStateDesc()
            : Name("GraphicsPSO")
        {
            this->pRootSignature = nullptr;
            this->VS = CD3DX12_PIPELINE_STATE_STREAM_VS();
            this->PS = CD3DX12_PIPELINE_STATE_STREAM_PS();
            this->DS = CD3DX12_PIPELINE_STATE_STREAM_DS();
            this->HS = CD3DX12_PIPELINE_STATE_STREAM_HS();
            this->GS = CD3DX12_PIPELINE_STATE_STREAM_GS();
            this->StreamOutput = CD3DX12_PIPELINE_STATE_STREAM_STREAM_OUTPUT();
            this->BlendState = CD3DX12_BLEND_DESC(CD3DX12_DEFAULT());
            this->SampleMask = 0xFFFFFFFFu;
            this->RasterizerState = CD3DX12_RASTERIZER_DESC(CD3DX12_DEFAULT());
            this->DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(CD3DX12_DEFAULT());
            this->InputLayout.NumElements = 0;
            this->InputLayout.pInputElementDescs = nullptr;
            this->IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
            this->PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            this->NumRenderTargets = 0;
            for (DXGI_FORMAT& rtvFormat : RTVFormats)
            {
                rtvFormat = DXGI_FORMAT_UNKNOWN;
            }
            this->DSVFormat = DXGI_FORMAT_UNKNOWN;
            this->SampleDesc = CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC();
            this->NodeMask = 0;
            this->CachedPSO = CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO();
            this->Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
        }

        void SetVertexShader(ShaderBlob& vertexShader);
        void SetPixelShader(ShaderBlob& pixelShader);
        void SetDomainShader(ShaderBlob& domainShader);
        void SetHullShader(ShaderBlob& hullShader);
        void SetGeometryShader(ShaderBlob& geometryShader);
        void SetRootSignature(RootSignature& rootSignature);

    public:
        String Name;
    };

    class ComputePipelineStateDesc : public D3D12_COMPUTE_PIPELINE_STATE_DESC
    {
    public:
        ComputePipelineStateDesc()
            : Name("ComputePSO")
        {
            this->pRootSignature = nullptr;
            this->CS = CD3DX12_PIPELINE_STATE_STREAM_CS();
            this->NodeMask = 0;
            this->CachedPSO = CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO();
            this->Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
        }

        void SetComputeShader(ShaderBlob& computeShader);

    public:
        String Name;
    };

} // namespace fe