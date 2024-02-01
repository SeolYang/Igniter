#include <D3D12/PipelineStateDesc.h>
#include <D3D12/ShaderBlob.h>

namespace fe::dx
{
	void GraphicsPipelineStateDesc::SetVertexShader(ShaderBlob& vertexShader)
	{
		verify(vertexShader.GetType() == EShaderType::Vertex);

		auto& nativeBlob = vertexShader.GetNative();
		verify(nativeBlob.GetBufferPointer() != nullptr);
		verify(nativeBlob.GetBufferSize() > 0);
		this->VS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
	}

	void GraphicsPipelineStateDesc::SetPixelShader(ShaderBlob& pixelShader)
	{
		verify(pixelShader.GetType() == EShaderType::Pixel);

		auto& nativeBlob = pixelShader.GetNative();
		verify(nativeBlob.GetBufferPointer() != nullptr);
		verify(nativeBlob.GetBufferSize() > 0);
		this->PS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
	}

	void GraphicsPipelineStateDesc::SetDomainShader(ShaderBlob& domainShader)
	{
		verify(domainShader.GetType() == EShaderType::Domain);

		auto& nativeBlob = domainShader.GetNative();
		verify(nativeBlob.GetBufferPointer() != nullptr);
		verify(nativeBlob.GetBufferSize() > 0);
		this->DS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
	}

	void GraphicsPipelineStateDesc::SetHullShader(ShaderBlob& hullShader)
	{
		verify(hullShader.GetType() == EShaderType::Hull);

		auto& nativeBlob = hullShader.GetNative();
		verify(nativeBlob.GetBufferPointer() != nullptr);
		verify(nativeBlob.GetBufferSize() > 0);
		this->HS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
	}

	void GraphicsPipelineStateDesc::SetGeometryShader(ShaderBlob& geometryShader)
	{
		verify(geometryShader.GetType() == EShaderType::Geometry);

		auto& nativeBlob = geometryShader.GetNative();
		verify(nativeBlob.GetBufferPointer() != nullptr);
		verify(nativeBlob.GetBufferSize() > 0);
		this->GS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
	}

	void ComputePipelineStateDesc::SetComputeShader(ShaderBlob& computeShader)
	{
		verify(computeShader.GetType() == EShaderType::Compute);

		auto& nativeBlob = computeShader.GetNative();
		verify(nativeBlob.GetBufferPointer() != nullptr);
		verify(nativeBlob.GetBufferSize() > 0);
		this->CS = CD3DX12_SHADER_BYTECODE(nativeBlob.GetBufferPointer(), nativeBlob.GetBufferSize());
	}
} // namespace fe::dx