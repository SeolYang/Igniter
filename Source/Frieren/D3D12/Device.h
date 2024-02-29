#pragma once
#include <D3D12/Common.h>
#include <Core/Container.h>

namespace fe::dx
{
	class CommandQueue;
	class CommandContext;
	class DescriptorHeap;
	class GpuView;
	class GpuBufferDesc;
	class GpuBuffer;
	class GPUTextureDesc;
	class GpuTexture;
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

		[[nodiscard]] auto& GetNative() { return *device.Get(); }
		uint32_t GetDescriptorHandleIncrementSize(const EDescriptorHeapType type) const;

		std::optional<Fence> CreateFence(const std::string_view debugName, const uint64_t initialCounter = 0);
		std::optional<CommandQueue> CreateCommandQueue(const std::string_view debugName, const EQueueType queueType);
		std::optional<CommandContext> CreateCommandContext(const std::string_view debugName, const EQueueType targetQueueType);

		std::optional<RootSignature> CreateBindlessRootSignature();
		std::optional<PipelineState> CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc);
		std::optional<PipelineState> CreateComputePipelineState(const ComputePipelineStateDesc& desc);

		std::optional<GpuBuffer> CreateBuffer(const GpuBufferDesc& bufferDesc);
		std::optional<GpuTexture> CreateTexture(const GPUTextureDesc& textureDesc);

		std::optional<DescriptorHeap> CreateDescriptorHeap(const EDescriptorHeapType descriptorHeapType, const uint32_t numDescriptors);

		void UpdateConstantBufferView(const GpuView& gpuView, GpuBuffer& buffer);
		void UpdateConstantBufferView(const GpuView& gpuView, GpuBuffer& buffer, const uint64_t offset, const uint64_t sizeInBytes);
		void UpdateShaderResourceView(const GpuView& gpuView, GpuBuffer& buffer);
		void UpdateUnorderedAccessView(const GpuView& gpuView, GpuBuffer& buffer);

		void UpdateShaderResourceView(const GpuView& gpuView, GpuTexture& texture, const GpuViewTextureSubresource& subresource);
		void UpdateUnorderedAccessView(const GpuView& gpuView, GpuTexture& texture, const GpuViewTextureSubresource& subresource);
		void UpdateRenderTargetView(const GpuView& gpuView, GpuTexture& texture, const GpuViewTextureSubresource& subresource);
		void UpdateDepthStencilView(const GpuView& gpuView, GpuTexture& texture, const GpuViewTextureSubresource& subresource);

		ComPtr<D3D12MA::Pool> CreateCustomMemoryPool(const D3D12MA::POOL_DESC& desc);

	private:
		bool AcquireAdapterFromFactory();
		void LogAdapterInformations();
		bool CreateDevice();
		void SetSeverityLevel();
		void CheckSupportedFeatures();
		void CacheDescriptorHandleIncrementSize();
		bool CreateMemoryAllcator();

	private:
		ComPtr<IDXGIAdapter> adapter;
		ComPtr<ID3D12Device10> device;

		D3D12MA::Allocator* allocator = nullptr;

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