#include <D3D12/PipelineStateDesc.h>
#include <D3D12/ShaderBlob.h>
#include <D3D12/RootSignature.h>
#include <Core/Assert.h>

namespace fe
{
	void GraphicsPipelineStateDesc::SetVertexShader(ShaderBlob& vertexShader)
	{
		check(vertexShader.GetType() == EShaderType::Vertex);

		auto& nativeBlob = vertexShader.GetNative();
		check(nativeBlob.GetBufferPointer() != nullptr);
		check(nativeBlob.GetBufferSize() > 0);
		this->VS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
	}

	void GraphicsPipelineStateDesc::SetPixelShader(ShaderBlob& pixelShader)
	{
		check(pixelShader.GetType() == EShaderType::Pixel);

		auto& nativeBlob = pixelShader.GetNative();
		check(nativeBlob.GetBufferPointer() != nullptr);
		check(nativeBlob.GetBufferSize() > 0);
		this->PS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
	}

	void GraphicsPipelineStateDesc::SetDomainShader(ShaderBlob& domainShader)
	{
		check(domainShader.GetType() == EShaderType::Domain);

		auto& nativeBlob = domainShader.GetNative();
		check(nativeBlob.GetBufferPointer() != nullptr);
		check(nativeBlob.GetBufferSize() > 0);
		this->DS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
	}

	void GraphicsPipelineStateDesc::SetHullShader(ShaderBlob& hullShader)
	{
		check(hullShader.GetType() == EShaderType::Hull);

		auto& nativeBlob = hullShader.GetNative();
		check(nativeBlob.GetBufferPointer() != nullptr);
		check(nativeBlob.GetBufferSize() > 0);
		this->HS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
	}

	void GraphicsPipelineStateDesc::SetGeometryShader(ShaderBlob& geometryShader)
	{
		check(geometryShader.GetType() == EShaderType::Geometry);

		auto& nativeBlob = geometryShader.GetNative();
		check(nativeBlob.GetBufferPointer() != nullptr);
		check(nativeBlob.GetBufferSize() > 0);
		this->GS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
	}

	void GraphicsPipelineStateDesc::SetRootSignature(RootSignature& rootSignature)
	{
		check(rootSignature);
		this->pRootSignature = &rootSignature.GetNative();
	}

	void ComputePipelineStateDesc::SetComputeShader(ShaderBlob& computeShader)
	{
		check(computeShader.GetType() == EShaderType::Compute);

		auto& nativeBlob = computeShader.GetNative();
		check(nativeBlob.GetBufferPointer() != nullptr);
		check(nativeBlob.GetBufferSize() > 0);
		this->CS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
	}
} // namespace fe