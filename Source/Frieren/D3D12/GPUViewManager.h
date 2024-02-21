#pragma once
#include <D3D12/Common.h>
#include <D3D12/GPUView.h>
#include <Core/Handle.h>

namespace fe
{
	class HandleManager;
	class DeferredDeallocator;
} // namespace fe

namespace fe::dx
{
	class GPUBuffer;
	class GPUTexture;
	class Device;
	class DescriptorHeap;
	class GPUViewManager;
	// #todo 아예 GPUViewManager로 합쳐버리기 -> UniqueHandle Destroyer 좀더 손보기


	// #wip_features
	class GPUViewManager
	{
		friend class GPUViewHandleDestroyer;

	public:
		GPUViewManager(HandleManager& handleManager, DeferredDeallocator& deferredDeallocator, Device& device);
		GPUViewManager(const GPUViewManager&) = delete;
		GPUViewManager(GPUViewManager&&) noexcept = delete;
		~GPUViewManager();

		GPUViewManager& operator=(const GPUViewManager&) = delete;
		GPUViewManager& operator=(GPUViewManager&&) noexcept = delete;

		std::array<std::reference_wrapper<DescriptorHeap>, 2> GetBindlessDescriptorHeaps();

		auto RequestConstantBufferView(GPUBuffer& gpuBuffer)
		{
			std::optional<GPUView> newCBV = Allocate(EGPUViewType::ConstantBufferView);
			if (newCBV)
			{
				check(newCBV->Type == EGPUViewType::ConstantBufferView);
				check(newCBV->Index != InvalidIndexU32);
				UpdateShaderResourceView(*newCBV, gpuBuffer);
			}
			else
			{
				checkNoEntry();
			}

			return MakeHandle(*newCBV);
		}

		auto RequestShaderResourceView(GPUBuffer& gpuBuffer)
		{
			std::optional<GPUView> newSRV = Allocate(EGPUViewType::ShaderResourceView);
			if (newSRV)
			{
				check(newSRV->Type == EGPUViewType::ShaderResourceView);
				check(newSRV->Index != InvalidIndexU32);
				UpdateShaderResourceView(*newSRV, gpuBuffer);
			}
			else
			{
				checkNoEntry();
			}

			return MakeHandle(*newSRV);
		}

		auto RequestUnorderedAccessView(GPUBuffer& gpuBuffer)
		{
			std::optional<GPUView> newUAV = Allocate(EGPUViewType::UnorderedAccessView);
			if (newUAV)
			{
				check(newUAV->Type == EGPUViewType::UnorderedAccessView);
				check(newUAV->Index != InvalidIndexU32);
				UpdateUnorderedAccessView(*newUAV, gpuBuffer);
			}
			else
			{
				checkNoEntry();
			}

			return MakeHandle(*newUAV);
		}

		auto RequestShaderResourceView(GPUTexture& gpuTexture, const GPUTextureSubresource& subresource)
		{
			std::optional<GPUView> newSRV = Allocate(EGPUViewType::ShaderResourceView);
			if (newSRV)
			{
				check(newSRV->Type == EGPUViewType::ShaderResourceView);
				check(newSRV->Index != InvalidIndexU32);
				UpdateShaderResourceView(*newSRV, gpuTexture, subresource);
			}
			else
			{
				checkNoEntry();
			}

			return MakeHandle(*newSRV);
		}

		auto RequestUnorderedAccessView(GPUTexture& gpuTexture, const GPUTextureSubresource& subresource)
		{
			std::optional<GPUView> newUAV = Allocate(EGPUViewType::UnorderedAccessView);
			if (newUAV)
			{
				check(newUAV->Type == EGPUViewType::UnorderedAccessView);
				check(newUAV->Index != InvalidIndexU32);
				UpdateUnorderedAccessView(*newUAV, gpuTexture, subresource);
			}
			else
			{
				checkNoEntry();
			}

			return MakeHandle(*newUAV);
		}

		auto RequestRenderTargetView(GPUTexture& gpuTexture, const GPUTextureSubresource& subresource)
		{
			std::optional<GPUView> newRTV = Allocate(EGPUViewType::RenderTargetView);
			if (newRTV)
			{
				check(newRTV->Type == EGPUViewType::RenderTargetView);
				check(newRTV->Index != InvalidIndexU32);
				UpdateRenderTargetView(*newRTV, gpuTexture, subresource);
			}
			else
			{
				checkNoEntry();
			}

			return MakeHandle(*newRTV);
		}

		auto RequestDepthStencilView(GPUTexture& gpuTexture, const GPUTextureSubresource& subresource)
		{
			std::optional<GPUView> newDSV = Allocate(EGPUViewType::DepthStencilView);
			if (newDSV)
			{
				check(newDSV->Type == EGPUViewType::DepthStencilView);
				check(newDSV->Index != InvalidIndexU32);
				UpdateDepthStencilView(*newDSV, gpuTexture, subresource);
			}
			else
			{
				checkNoEntry();
			}

			return MakeHandle(*newDSV);
		}

	private:
		UniqueHandle<GPUView, GPUViewHandleDestroyer> MakeHandle(const GPUView view);

		std::optional<GPUView> Allocate(const EGPUViewType type);
		void				   Deallocate(const GPUView& gpuView);

		void UpdateConstantBufferView(const GPUView& gpuView, GPUBuffer& buffer);
		void UpdateShaderResourceView(const GPUView& gpuView, GPUBuffer& buffer);
		void UpdateUnorderedAccessView(const GPUView& gpuView, GPUBuffer& buffer);

		void UpdateShaderResourceView(const GPUView& gpuView, GPUTexture& gpuTexture, const GPUTextureSubresource& subresource);
		void UpdateUnorderedAccessView(const GPUView& gpuView, GPUTexture& gpuTexture, const GPUTextureSubresource& subresource);
		void UpdateRenderTargetView(const GPUView& gpuView, GPUTexture& gpuTexture, const GPUTextureSubresource& subresource);
		void UpdateDepthStencilView(const GPUView& gpuView, GPUTexture& gpuTexture, const GPUTextureSubresource& subresource);

	private:
		HandleManager&		 handleManager;
		DeferredDeallocator& deferredDeallocator;
		Device&				 device;

		std::unique_ptr<DescriptorHeap> cbvSrvUavHeap;
		std::unique_ptr<DescriptorHeap> samplerHeap;
		std::unique_ptr<DescriptorHeap> rtvHeap;
		std::unique_ptr<DescriptorHeap> dsvHeap;

		static constexpr uint32_t NumCbvSrvUavDescriptors = 2048;
		static constexpr uint32_t NumSamplerDescriptors = 512;
		static constexpr uint32_t NumRtvDescriptors = 256;
		static constexpr uint32_t NumDsvDescriptors = NumRtvDescriptors;
	};
} // namespace fe::dx