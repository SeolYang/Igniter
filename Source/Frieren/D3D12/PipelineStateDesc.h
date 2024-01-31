#pragma once
#include <D3D12/Common.h>
#include <Core/String.h>
#include <span>

namespace fe::dx
{
	class ShaderBlob;
	class GraphicsPipelineStateDesc : public D3D12_GRAPHICS_PIPELINE_STATE_DESC
	{
	public:
		GraphicsPipelineStateDesc() : Name("GraphicsPSO")
		{
			this->NodeMask = 0;
			this->pRootSignature = nullptr;
			this->SampleMask = 0xFFFFFFFFu;
			this->SampleDesc.Count = 1;
			this->SampleDesc.Quality = 0;
			this->InputLayout.NumElements = 0;
		}

		void SetVertexShader(const ShaderBlob& vertexShader);
		void SetPixelShader(const ShaderBlob& pixelShader);
		void SetDomainShader(const ShaderBlob& domainShader);
		void SetHullShader(const ShaderBlob& hullShader);
		void SetGeometryShader(const ShaderBlob& geometryShader);

	public:
		String Name;
	};

	class ComputePipelineStateDesc : public D3D12_COMPUTE_PIPELINE_STATE_DESC
	{
	public:
		ComputePipelineStateDesc() : Name("ComputePSO")
		{
			this->NodeMask = 0;
			this->pRootSignature = nullptr;
		}

		void SetComputeShader(const ShaderBlob& computeShader);

	public:
		String Name;
	};

} // namespace fe::dx