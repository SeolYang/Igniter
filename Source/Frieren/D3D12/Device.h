#pragma once
#include <Core/Log.h>
#include <Core/HandleManager.h>
#include <D3D12/Commons.h>
#include <D3D12/GPUBufferDescription.h>

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
	class GPUBuffer;
	class GPUTexture;
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

		uint32_t GetCbvSrvUavDescriptorHandleIncrementSize() const { return cbvSrvUavDescriptorHandleIncrementSize; }
		uint32_t GetSamplerDescriptorHandleIncrementSize() const { return samplerDescritorHandleIncrementSize; }
		uint32_t GetDsvDescriptorHandleIncrementSize() const { return dsvDescriptorHandleIncrementSize; }
		uint32_t GetRtvDescriptorHandleIncrementSize() const { return rtvDescriptorHandleIncrementSize; }
		uint32_t GetDescriptorHandleIncrementSize(const D3D12_DESCRIPTOR_HEAP_TYPE type)
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

		void FlushQueue(D3D12_COMMAND_LIST_TYPE queueType);
		void FlushGPU();

		ID3D12Device10& GetNative() const { return *device.Get(); }

		GPUBuffer CreateBuffer(const GPUBufferDesc description);
		// GPUTexture BuildGPUTexture(GPUTextureBuildDescription);

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