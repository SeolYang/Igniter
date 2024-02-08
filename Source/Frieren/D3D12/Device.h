#pragma once
#include <Core/Log.h>
#include <Core/HandleManager.h>
#include <D3D12/Common.h>

namespace fe::dx
{
	FE_DECLARE_LOG_CATEGORY(D3D12Info, ELogVerbosiy::Info);
	FE_DECLARE_LOG_CATEGORY(D3D12Warn, ELogVerbosiy::Warning);
	FE_DECLARE_LOG_CATEGORY(D3D12Fatal, ELogVerbosiy::Fatal);

	class DescriptorHeap;
	class GPUBufferDesc;
	class GPUBuffer;
	class GPUTextureDesc;
	class GPUTextureSubresource;
	class GPUTexture;
	class Descriptor;
	class CommandContext;
	class Device
	{
	public:
		Device();
		~Device();

		Device(Device&) = delete;
		Device(Device&&) noexcept = delete;

		Device& operator=(Device&) = delete;
		Device& operator=(Device&&) noexcept = delete;

		auto&				GetDirectQueue() { return *directQueue.Get(); }
		auto&				GetAsyncComputeQueue() { return *asyncComputeQueue.Get(); }
		auto&				GetCopyQueue() { return *copyQueue.Get(); }
		[[nodiscard]] auto& GetNative() { return *device.Get(); }

		uint32_t GetCbvSrvUavDescriptorHandleIncrementSize() const { return cbvSrvUavDescriptorHandleIncrementSize; }
		uint32_t GetSamplerDescriptorHandleIncrementSize() const { return samplerDescritorHandleIncrementSize; }
		uint32_t GetDsvDescriptorHandleIncrementSize() const { return dsvDescriptorHandleIncrementSize; }
		uint32_t GetRtvDescriptorHandleIncrementSize() const { return rtvDescriptorHandleIncrementSize; }
		uint32_t GetDescriptorHandleIncrementSize(const D3D12_DESCRIPTOR_HEAP_TYPE type) const
		{
			switch (type)
			{
				case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
					return cbvSrvUavDescriptorHandleIncrementSize;
				case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
					return samplerDescritorHandleIncrementSize;
				case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
					return dsvDescriptorHandleIncrementSize;
				case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
					return rtvDescriptorHandleIncrementSize;
				default:
					return 0;
			}
		}

		// #todo Execute Command Context	(Immediate)
		// #todo Execute Command Contexts	(Immediate)

		void FlushQueue(D3D12_COMMAND_LIST_TYPE queueType);
		void FlushGPU();

		std::optional<GPUBuffer>  CreateBuffer(const GPUBufferDesc& bufferDesc);
		std::optional<GPUTexture> CreateTexture(const GPUTextureDesc& textureDesc);

		std::optional<Descriptor> CreateShaderResourceView(GPUBuffer& gpuBuffer);
		std::optional<Descriptor> CreateConstantBufferView(GPUBuffer& gpuBuffer);
		std::optional<Descriptor> CreateUnorderedAccessView(GPUBuffer& gpuBuffer);
		std::optional<Descriptor> CreateShaderResourceView(GPUTexture&				   texture,
														   const GPUTextureSubresource subresource);
		std::optional<Descriptor> CreateUnorderedAccessView(GPUTexture&					texture,
															const GPUTextureSubresource subresource);
		std::optional<Descriptor> CreateRenderTargetView(GPUTexture& texture, const GPUTextureSubresource subresource);
		std::optional<Descriptor> CreateDepthStencilView(GPUTexture& texture, const GPUTextureSubresource subresource);

	private:
		bool AcquireAdapterFromFactory();
		void LogAdapterInformations();
		bool CreateDevice();
		void SetSeverityLevel();
		void CheckSupportedFeatures();
		void CacheDescriptorHandleIncrementSize();
		bool CreateMemoryAllcator();
		bool CreateCommandQueues();
		void CreateDescriptorHeaps();

	public:
		static constexpr uint32_t NumCbvSrvUavDescriptors = 2048;
		static constexpr uint32_t NumSamplerDescriptors = 512;
		static constexpr uint32_t NumRtvDescriptors = 256;
		static constexpr uint32_t NumDsvDescriptors = NumRtvDescriptors;

	private:
		ComPtr<IDXGIAdapter>   adapter;
		ComPtr<ID3D12Device10> device;

		ComPtr<ID3D12CommandQueue> directQueue;
		ComPtr<ID3D12CommandQueue> asyncComputeQueue;
		ComPtr<ID3D12CommandQueue> copyQueue;

		uint32_t cbvSrvUavDescriptorHandleIncrementSize = 0;
		uint32_t samplerDescritorHandleIncrementSize = 0;
		uint32_t dsvDescriptorHandleIncrementSize = 0;
		uint32_t rtvDescriptorHandleIncrementSize = 0;

		D3D12MA::Allocator* allocator = nullptr;

		std::unique_ptr<DescriptorHeap> cbvSrvUavDescriptorHeap;
		std::unique_ptr<DescriptorHeap> samplerDescriptorHeap;
		std::unique_ptr<DescriptorHeap> rtvDescriptorHeap;
		std::unique_ptr<DescriptorHeap> dsvDescriptorHeap;
	};
} // namespace fe::dx