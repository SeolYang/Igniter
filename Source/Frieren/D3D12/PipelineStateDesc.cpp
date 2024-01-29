#include <D3D12/PipelineStateDesc.h>
#include <D3D12/ShaderBlob.h>

namespace fe::dx
{
	void GraphicsPipelineStateDesc::SetVertexShader(const ShaderBlob& vertexShader)
	{
		verify(vertexShader.GetType() == EShaderType::Vertex);

		IDxcBlob& nativeBlob = vertexShader.GetNative();
		verify(nativeBlob.GetBufferPointer() != nullptr);
		verify(nativeBlob.GetBufferSize() > 0);
		this->VS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
	}

	void GraphicsPipelineStateDesc::SetPixelShader(const ShaderBlob& pixelShader)
	{
		verify(pixelShader.GetType() == EShaderType::Pixel);

		IDxcBlob& nativeBlob = pixelShader.GetNative();
		verify(nativeBlob.GetBufferPointer() != nullptr);
		verify(nativeBlob.GetBufferSize() > 0);
		this->PS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
	}

	void GraphicsPipelineStateDesc::SetDomainShader(const ShaderBlob& domainShader)
	{
		verify(domainShader.GetType() == EShaderType::Domain);

		IDxcBlob& nativeBlob = domainShader.GetNative();
		verify(nativeBlob.GetBufferPointer() != nullptr);
		verify(nativeBlob.GetBufferSize() > 0);
		this->DS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
	}

	void GraphicsPipelineStateDesc::SetHullShader(const ShaderBlob& hullShader)
	{
		verify(hullShader.GetType() == EShaderType::Hull);

		IDxcBlob& nativeBlob = hullShader.GetNative();
		verify(nativeBlob.GetBufferPointer() != nullptr);
		verify(nativeBlob.GetBufferSize() > 0);
		this->HS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
	}

	void GraphicsPipelineStateDesc::SetGeometryShader(const ShaderBlob& geometryShader)
	{
		verify(geometryShader.GetType() == EShaderType::Geometry);

		IDxcBlob& nativeBlob = geometryShader.GetNative();
		verify(nativeBlob.GetBufferPointer() != nullptr);
		verify(nativeBlob.GetBufferSize() > 0);
		this->GS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
	}

	void ComputePipelineStateDesc::SetComputeShader(const ShaderBlob& computeShader)
	{
		verify(computeShader.GetType() == EShaderType::Compute);

		IDxcBlob& nativeBlob = computeShader.GetNative();
		verify(nativeBlob.GetBufferPointer() != nullptr);
		verify(nativeBlob.GetBufferSize() > 0);
		this->CS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
	}
} // namespace fe::dx