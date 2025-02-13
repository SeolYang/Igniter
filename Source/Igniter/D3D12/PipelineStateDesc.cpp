#include "Igniter/Igniter.h"
#include "Igniter/D3D12/PipelineStateDesc.h"
#include "Igniter/D3D12/ShaderBlob.h"
#include "Igniter/D3D12/RootSignature.h"

namespace ig
{
    void GraphicsPipelineStateDesc::SetVertexShader(ShaderBlob& vertexShader)
    {
        IG_CHECK(vertexShader.GetType() == EShaderType::Vertex);

        auto& nativeBlob = vertexShader.GetNative();
        IG_CHECK(nativeBlob.GetBufferPointer() != nullptr);
        IG_CHECK(nativeBlob.GetBufferSize() > 0);
        this->VS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
    }

    void GraphicsPipelineStateDesc::SetPixelShader(ShaderBlob& pixelShader)
    {
        IG_CHECK(pixelShader.GetType() == EShaderType::Pixel);

        auto& nativeBlob = pixelShader.GetNative();
        IG_CHECK(nativeBlob.GetBufferPointer() != nullptr);
        IG_CHECK(nativeBlob.GetBufferSize() > 0);
        this->PS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
    }

    void GraphicsPipelineStateDesc::SetDomainShader(ShaderBlob& domainShader)
    {
        IG_CHECK(domainShader.GetType() == EShaderType::Domain);

        auto& nativeBlob = domainShader.GetNative();
        IG_CHECK(nativeBlob.GetBufferPointer() != nullptr);
        IG_CHECK(nativeBlob.GetBufferSize() > 0);
        this->DS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
    }

    void GraphicsPipelineStateDesc::SetHullShader(ShaderBlob& hullShader)
    {
        IG_CHECK(hullShader.GetType() == EShaderType::Hull);

        auto& nativeBlob = hullShader.GetNative();
        IG_CHECK(nativeBlob.GetBufferPointer() != nullptr);
        IG_CHECK(nativeBlob.GetBufferSize() > 0);
        this->HS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
    }

    void GraphicsPipelineStateDesc::SetGeometryShader(ShaderBlob& geometryShader)
    {
        IG_CHECK(geometryShader.GetType() == EShaderType::Geometry);

        auto& nativeBlob = geometryShader.GetNative();
        IG_CHECK(nativeBlob.GetBufferPointer() != nullptr);
        IG_CHECK(nativeBlob.GetBufferSize() > 0);
        this->GS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
    }

    void GraphicsPipelineStateDesc::SetRootSignature(RootSignature& rootSignature)
    {
        IG_CHECK(rootSignature);
        this->pRootSignature = &rootSignature.GetNative();
    }

    void ComputePipelineStateDesc::SetComputeShader(ShaderBlob& computeShader)
    {
        IG_CHECK(computeShader.GetType() == EShaderType::Compute);

        auto& nativeBlob = computeShader.GetNative();
        IG_CHECK(nativeBlob.GetBufferPointer() != nullptr);
        IG_CHECK(nativeBlob.GetBufferSize() > 0);
        this->CS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
    }

    void ComputePipelineStateDesc::SetRootSignature(RootSignature& rootSignature)
    {
        IG_CHECK(rootSignature);
        this->pRootSignature = &rootSignature.GetNative();
    }

    void MeshShaderPipelineStateDesc::SetRootSignature(RootSignature& rootSignature)
    {
        IG_CHECK(rootSignature);
        this->pRootSignature = &rootSignature.GetNative();
    }
    
    void MeshShaderPipelineStateDesc::SetAmplificationShader(ShaderBlob& amplitificationShader)
    {
        IG_CHECK(amplitificationShader.GetType() == EShaderType::Amplification);

        auto& nativeBlob = amplitificationShader.GetNative();
        IG_CHECK(nativeBlob.GetBufferPointer() != nullptr);
        IG_CHECK(nativeBlob.GetBufferSize() > 0);
        this->AS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
    }
    
    void MeshShaderPipelineStateDesc::SetMeshShader(ShaderBlob& meshShader)
    {
        IG_CHECK(meshShader.GetType() == EShaderType::Mesh);

        auto& nativeBlob = meshShader.GetNative();
        IG_CHECK(nativeBlob.GetBufferPointer() != nullptr);
        IG_CHECK(nativeBlob.GetBufferSize() > 0);
        this->MS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
    }
    
    void MeshShaderPipelineStateDesc::SetPixelShader(ShaderBlob& pixelShader)
    {
        IG_CHECK(pixelShader.GetType() == EShaderType::Pixel);

        auto& nativeBlob = pixelShader.GetNative();
        IG_CHECK(nativeBlob.GetBufferPointer() != nullptr);
        IG_CHECK(nativeBlob.GetBufferSize() > 0);
        this->PS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
    }
} // namespace ig
