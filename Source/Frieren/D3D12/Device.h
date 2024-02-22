#pragma once
#include <D3D12/Common.h>
#include <Core/Container.h>

namespace fe::dx
{
	class CommandQueue;
	class CommandContext;
	class DescriptorHeap;
	class GPUView;
	class GPUBufferDesc;
	class GPUBuffer;
	class GPUTextureDesc;
	class GPUTexture;
	class Fence;
	class GraphicsPipelineStateDesc;
	class ComputePipelineStateDesc;
	class PipelineState;
	class RootSignature;
	class Device
	{
	public:
		Device();
		~Device();

		Device(const Device&) = delete;
		Device(Device&&) noexcept = delete;

		Device& operator=(const Device&) = delete;
		Device& operator=(Device&&) noexcept = delete;

		[[nodiscard]] auto&									  GetNative() { return *device.Get(); }
		uint32_t											  GetDescriptorHandleIncrementSize(const EDescriptorHeapType type) const;

		std::optional<Fence>		  CreateFence(const std::string_view debugName, const uint64_t initialCounter = 0);
		std::optional<CommandQueue>	  CreateCommandQueue(const EQueueType queueType);
		std::optional<CommandContext> CreateCommandContext(const EQueueType targetQueueType);

		// #todo 일반화된 루트 시그니처 생성 및 캐싱 구현(해시 기반)
		std::optional<RootSignature> CreateBindlessRootSignature();
		std::optional<PipelineState> CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc);
		std::optional<PipelineState> CreateComputePipelineState(const ComputePipelineStateDesc& desc);

		std::optional<GPUBuffer>  CreateBuffer(const GPUBufferDesc& bufferDesc);
		std::optional<GPUTexture> CreateTexture(const GPUTextureDesc& textureDesc);

		std::optional<DescriptorHeap> CreateDescriptorHeap(const EDescriptorHeapType descriptorHeapType, const uint32_t numDescriptors);

		void UpdateConstantBufferView(const GPUView& gpuView, GPUBuffer& buffer);
		void UpdateShaderResourceView(const GPUView& gpuView, GPUBuffer& buffer);
		void UpdateUnorderedAccessView(const GPUView& gpuView, GPUBuffer& buffer);

		void UpdateShaderResourceView(const GPUView& gpuView, GPUTexture& texture, const GPUTextureSubresource& subresource);
		void UpdateUnorderedAccessView(const GPUView& gpuView, GPUTexture& texture, const GPUTextureSubresource& subresource);
		void UpdateRenderTargetView(const GPUView& gpuView, GPUTexture& texture, const GPUTextureSubresource& subresource);
		void UpdateDepthStencilView(const GPUView& gpuView, GPUTexture& texture, const GPUTextureSubresource& subresource);

	private:
		bool AcquireAdapterFromFactory();
		void LogAdapterInformations();
		bool CreateDevice();
		void SetSeverityLevel();
		void CheckSupportedFeatures();
		void CacheDescriptorHandleIncrementSize();
		bool CreateMemoryAllcator();

	private:
		ComPtr<IDXGIAdapter>   adapter;
		ComPtr<ID3D12Device10> device;

		D3D12MA::Allocator*				allocator = nullptr;

		uint32_t cbvSrvUavDescriptorHandleIncrementSize = 0;
		uint32_t samplerDescritorHandleIncrementSize = 0;
		uint32_t dsvDescriptorHandleIncrementSize = 0;
		uint32_t rtvDescriptorHandleIncrementSize = 0;

		bool bEnhancedBarriersSupported = false;
		bool bRaytracing10Supported = false;
		bool bRaytracing11Supported = false;
		bool bShaderModel66Supported = false;
	};
} // namespace fe::dx