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

		void SetVertexShader(ShaderBlob& vertexShader);
		void SetPixelShader(ShaderBlob& pixelShader);
		void SetDomainShader(ShaderBlob& domainShader);
		void SetHullShader(ShaderBlob& hullShader);
		void SetGeometryShader(ShaderBlob& geometryShader);

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

		void SetComputeShader(ShaderBlob& computeShader);
	public:
		String Name;
	};

} // namespace fe::dx