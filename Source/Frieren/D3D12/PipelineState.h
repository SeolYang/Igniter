#pragma once
#include <D3D12/Common.h>
#include <Core/String.h>
#include <Core/Mutex.h>

namespace fe::dx
{
	class Device;
	class GraphicsPipelineStateDesc;
	class ComputePipelineStateDesc;
	// class PSOCache;
	//  PipelineStateDesc -> private static 캐시를 이용한 flyweight 패턴 구현
	//  buffer, texture 는 내부 데이터가 다를 수 있기 때문에 flyweight 불가능
	class PipelineState
	{
	public:
		PipelineState(const Device& device, const GraphicsPipelineStateDesc& desc);
		PipelineState(const Device& device, const ComputePipelineStateDesc& desc);
		~PipelineState();

		String				 GetName() const { return name; }
		ID3D12PipelineState& GetNative() const { return *pso.Get(); }

		bool IsGraphics() const { return bIsGraphicsPSO; }
		bool IsCompute() const { return !bIsGraphicsPSO; }

	private:
		const bool						 bIsGraphicsPSO;
		const String					 name;
		ComPtr<ID3D12PipelineState> pso;
	};
} // namespace fe