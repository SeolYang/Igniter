#pragma once
#include <Core/Log.h>
#include <Core/HandleManager.h>
#include <D3D12/Commons.h>

namespace fe
{
	FE_DECLARE_LOG_CATEGORY(D3D12Info, ELogVerbosiy::Info);
	FE_DECLARE_LOG_CATEGORY(D3D12Warn, ELogVerbosiy::Warning);
	FE_DECLARE_LOG_CATEGORY(D3D12Fatal, ELogVerbosiy::Fatal);

	inline bool IsDXCallSucceeded(const HRESULT result)
	{
		return result == S_OK;
	}

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

		Device(const Device&) = delete;
		Device(Device&&) noexcept = delete;

		Device& operator=(const Device&) = delete;
		Device& operator=(Device&&) noexcept = delete;

		ID3D12CommandQueue& GetDirectQueue() const { return *directQueue.Get(); }
		ID3D12CommandQueue& GetAsyncComputeQueue() const { return *asyncComputeQueue.Get(); }
		ID3D12CommandQueue& GetCopyQueue() const { return *copyQueue.Get(); }
		ID3D12Device10&		GetNative() const { return *device.Get(); }

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

		// #todo Descriptor -> BufferView = Descriptor + Reference to buffer?
		std::optional<Descriptor> CreateShaderResourceView(const GPUBuffer& gpuBuffer);
		std::optional<Descriptor> CreateConstantBufferView(const GPUBuffer& gpuBuffer);
		std::optional<Descriptor> CreateUnorderedAccessView(const GPUBuffer& gpuBuffer);

		// #todo Descriptor -> TextureView = Descriptor + Reference to texture + subresource + view format?
		std::optional<Descriptor> CreateShaderResourceView(const GPUTexture& texture, const GPUTextureSubresource subresource);
		std::optional<Descriptor> CreateUnorderedAccessView(const GPUTexture& texture, const GPUTextureSubresource subresource);
		std::optional<Descriptor> CreateRenderTargetView(const GPUTexture& texture, const GPUTextureSubresource subresource);
		std::optional<Descriptor> CreateDepthStencilView(const GPUTexture& texture, const GPUTextureSubresource subresource);

		GPUResource AllocateResource(const D3D12_RESOURCE_DESC1 resourceDesc, const D3D12MA::ALLOCATION_DESC allocationDesc);

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
		wrl::ComPtr<IDXGIAdapter>	adapter;
		wrl::ComPtr<ID3D12Device10> device;

		wrl::ComPtr<ID3D12CommandQueue> directQueue;
		wrl::ComPtr<ID3D12CommandQueue> asyncComputeQueue;
		wrl::ComPtr<ID3D12CommandQueue> copyQueue;

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
} // namespace fe