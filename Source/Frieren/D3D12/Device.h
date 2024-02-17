#pragma once
#include <Core/Log.h>
#include <Core/FrameResource.h>
#include <D3D12/Common.h>

namespace fe::dx
{
	FE_DECLARE_LOG_CATEGORY(D3D12Info, ELogVerbosiy::Info)
	FE_DECLARE_LOG_CATEGORY(D3D12Warn, ELogVerbosiy::Warning)
	FE_DECLARE_LOG_CATEGORY(D3D12Fatal, ELogVerbosiy::Fatal)

	class CommandQueue;
	class CommandContext;
	class DescriptorHeap;
	class GPUView;
	class GPUBufferDesc;
	class GPUBuffer;
	class GPUTextureDesc;
	struct GPUTextureSubresource;
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
		std::array<std::reference_wrapper<DescriptorHeap>, 2> GetBindlessDescriptorHeaps();
		uint32_t											  GetDescriptorHandleIncrementSize(const EDescriptorHeapType type) const;

		// #todo std::optional -> FrameResource
		std::optional<GPUBuffer>  CreateBuffer(const GPUBufferDesc& bufferDesc);
		std::optional<GPUTexture> CreateTexture(const GPUTextureDesc& textureDesc);

		std::optional<DescriptorHeap> CreateDescriptorHeap(const EDescriptorHeapType descriptorHeapType,
														   const uint32_t			 numDescriptors);

		FrameResource<GPUView> CreateConstantBufferView(FrameResourceManager& frameResourceManager, GPUBuffer& gpuBuffer);
		FrameResource<GPUView> CreateShaderResourceView(FrameResourceManager& frameResourceManager, GPUBuffer& gpuBuffer);
		FrameResource<GPUView> CreateUnorderedAccessView(FrameResourceManager& frameResourceManager, GPUBuffer& gpuBuffer);

		FrameResource<GPUView> CreateShaderResourceView(FrameResourceManager& frameResourceManager, GPUTexture& gpuTexture, const GPUTextureSubresource& subresource);
		FrameResource<GPUView> CreateUnorderedAccessView(FrameResourceManager& frameResourceManager, GPUTexture& gpuTexture, const GPUTextureSubresource& subresource);
		FrameResource<GPUView> CreateRenderTargetView(FrameResourceManager& frameResourceManager, GPUTexture& gpuTexture, const GPUTextureSubresource& subresource);
		FrameResource<GPUView> CreateDepthStencilView(FrameResourceManager& frameResourceManager, GPUTexture& gpuTexture, const GPUTextureSubresource& subresource);

		std::optional<Fence> CreateFence(const std::string_view debugName, const uint64_t initialCounter = 0);

		std::optional<PipelineState> CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc);
		std::optional<PipelineState> CreateComputePipelineState(const ComputePipelineStateDesc& desc);

		std::optional<RootSignature> CreateBindlessRootSignature();

		std::optional<CommandQueue>	  CreateCommandQueue(const EQueueType queueType);
		std::optional<CommandContext> CreateCommandContext(const EQueueType targetQueueType);

	private:
		bool AcquireAdapterFromFactory();
		void LogAdapterInformations();
		bool CreateDevice();
		void SetSeverityLevel();
		void CheckSupportedFeatures();
		void CacheDescriptorHandleIncrementSize();
		bool CreateMemoryAllcator();
		void CreateBindlessDescriptorHeaps();

	private:
		static constexpr uint32_t NumCbvSrvUavDescriptors = 2048;
		static constexpr uint32_t NumSamplerDescriptors = 512;
		static constexpr uint32_t NumRtvDescriptors = 256;
		static constexpr uint32_t NumDsvDescriptors = NumRtvDescriptors;

	private:
		ComPtr<IDXGIAdapter>   adapter;
		ComPtr<ID3D12Device10> device;

		D3D12MA::Allocator*				allocator = nullptr;
		std::unique_ptr<DescriptorHeap> cbvSrvUavDescriptorHeap;
		std::unique_ptr<DescriptorHeap> samplerDescriptorHeap;
		std::unique_ptr<DescriptorHeap> rtvDescriptorHeap;
		std::unique_ptr<DescriptorHeap> dsvDescriptorHeap;

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