#include <D3D12/PipelineStateDesc.h>
#include <D3D12/ShaderBlob.h>

namespace fe::dx
{
	void GraphicsPipelineStateDesc::SetVertexShader(const ShaderBlob& vertexShader)
	{
		FE_ASSERT(vertexShader.GetType() == EShaderType::Vertex);

		IDxcBlob& nativeBlob = vertexShader.GetNative();
		FE_ASSERT(nativeBlob.GetBufferPointer() != nullptr);
		FE_ASSERT(nativeBlob.GetBufferSize() > 0);
		this->VS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
	}

	void GraphicsPipelineStateDesc::SetPixelShader(const ShaderBlob& pixelShader)
	{
		FE_ASSERT(pixelShader.GetType() == EShaderType::Pixel);

		IDxcBlob& nativeBlob = pixelShader.GetNative();
		FE_ASSERT(nativeBlob.GetBufferPointer() != nullptr);
		FE_ASSERT(nativeBlob.GetBufferSize() > 0);
		this->PS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
	}

	void GraphicsPipelineStateDesc::SetDomainShader(const ShaderBlob& domainShader)
	{
		FE_ASSERT(domainShader.GetType() == EShaderType::Domain);

		IDxcBlob& nativeBlob = domainShader.GetNative();
		FE_ASSERT(nativeBlob.GetBufferPointer() != nullptr);
		FE_ASSERT(nativeBlob.GetBufferSize() > 0);
		this->DS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
	}

	void GraphicsPipelineStateDesc::SetHullShader(const ShaderBlob& hullShader)
	{
		FE_ASSERT(hullShader.GetType() == EShaderType::Hull);

		IDxcBlob& nativeBlob = hullShader.GetNative();
		FE_ASSERT(nativeBlob.GetBufferPointer() != nullptr);
		FE_ASSERT(nativeBlob.GetBufferSize() > 0);
		this->HS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
	}

	void GraphicsPipelineStateDesc::SetGeometryShader(const ShaderBlob& geometryShader)
	{
		FE_ASSERT(geometryShader.GetType() == EShaderType::Geometry);

		IDxcBlob& nativeBlob = geometryShader.GetNative();
		FE_ASSERT(nativeBlob.GetBufferPointer() != nullptr);
		FE_ASSERT(nativeBlob.GetBufferSize() > 0);
		this->GS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
	}

	void ComputePipelineStateDesc::SetComputeShader(const ShaderBlob& computeShader)
	{
		FE_ASSERT(computeShader.GetType() == EShaderType::Compute);

		IDxcBlob& nativeBlob = computeShader.GetNative();
		FE_ASSERT(nativeBlob.GetBufferPointer() != nullptr);
		FE_ASSERT(nativeBlob.GetBufferSize() > 0);
		this->CS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
	}
} // namespace fe::dx