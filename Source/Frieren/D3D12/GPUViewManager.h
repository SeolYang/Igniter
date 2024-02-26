#pragma once
#include <D3D12/Common.h>
#include <D3D12/GPUView.h>
#include <Core/Handle.h>

namespace fe
{
	class HandleManager;
	class DeferredDeallocator;
	void RequestDeferredDeallocation(DeferredDeallocator& deferredDeallocator, DefaultCallback requester);
} // namespace fe

namespace fe::dx
{
	class GpuBuffer;
	class GpuTexture;
	class Device;
	class DescriptorHeap;
	class GpuViewManager
	{
		friend class Handle<GpuView, GpuViewManager*>;

	public:
		GpuViewManager(HandleManager& handleManager, DeferredDeallocator& deferredDeallocator, Device& device);
		GpuViewManager(const GpuViewManager&) = delete;
		GpuViewManager(GpuViewManager&&) noexcept = delete;
		~GpuViewManager();

		GpuViewManager& operator=(const GpuViewManager&) = delete;
		GpuViewManager& operator=(GpuViewManager&&) noexcept = delete;

		std::array<std::reference_wrapper<DescriptorHeap>, 2> GetBindlessDescriptorHeaps();

		auto RequestConstantBufferView(GpuBuffer& gpuBuffer)
		{
			std::optional<GpuView> newCBV = Allocate(EGpuViewType::ConstantBufferView);
			if (newCBV)
			{
				check(newCBV->Type == EGpuViewType::ConstantBufferView);
				check(newCBV->Index != InvalidIndexU32);
				UpdateConstantBufferView(*newCBV, gpuBuffer);
			}
			else
			{
				checkNoEntry();
			}

			return MakeHandle(*newCBV);
		}

		auto RequestConstantBufferView(GpuBuffer& gpuBuffer, const uint64_t offset, const uint64_t sizeInBytes)
		{
			std::optional<GpuView> newCBV = Allocate(EGpuViewType::ConstantBufferView);
			if (newCBV)
			{
				check(newCBV->Type == EGpuViewType::ConstantBufferView);
				check(newCBV->Index != InvalidIndexU32);
				check(offset % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0);
				check(sizeInBytes % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0);
				UpdateConstantBufferView(*newCBV, gpuBuffer, offset, sizeInBytes);
			}
			else
			{
				checkNoEntry();
			}

			return MakeHandle(*newCBV);
		}

		auto RequestShaderResourceView(GpuBuffer& gpuBuffer)
		{
			std::optional<GpuView> newSRV = Allocate(EGpuViewType::ShaderResourceView);
			if (newSRV)
			{
				check(newSRV->Type == EGpuViewType::ShaderResourceView);
				check(newSRV->Index != InvalidIndexU32);
				UpdateShaderResourceView(*newSRV, gpuBuffer);
			}
			else
			{
				checkNoEntry();
			}

			return MakeHandle(*newSRV);
		}

		auto RequestUnorderedAccessView(GpuBuffer& gpuBuffer)
		{
			std::optional<GpuView> newUAV = Allocate(EGpuViewType::UnorderedAccessView);
			if (newUAV)
			{
				check(newUAV->Type == EGpuViewType::UnorderedAccessView);
				check(newUAV->Index != InvalidIndexU32);
				UpdateUnorderedAccessView(*newUAV, gpuBuffer);
			}
			else
			{
				checkNoEntry();
			}

			return MakeHandle(*newUAV);
		}

		auto RequestShaderResourceView(GpuTexture& gpuTexture, const GpuViewTextureSubresource& subresource)
		{
			std::optional<GpuView> newSRV = Allocate(EGpuViewType::ShaderResourceView);
			if (newSRV)
			{
				check(newSRV->Type == EGpuViewType::ShaderResourceView);
				check(newSRV->Index != InvalidIndexU32);
				UpdateShaderResourceView(*newSRV, gpuTexture, subresource);
			}
			else
			{
				checkNoEntry();
			}

			return MakeHandle(*newSRV);
		}

		auto RequestUnorderedAccessView(GpuTexture& gpuTexture, const GpuViewTextureSubresource& subresource)
		{
			std::optional<GpuView> newUAV = Allocate(EGpuViewType::UnorderedAccessView);
			if (newUAV)
			{
				check(newUAV->Type == EGpuViewType::UnorderedAccessView);
				check(newUAV->Index != InvalidIndexU32);
				UpdateUnorderedAccessView(*newUAV, gpuTexture, subresource);
			}
			else
			{
				checkNoEntry();
			}

			return MakeHandle(*newUAV);
		}

		auto RequestRenderTargetView(GpuTexture& gpuTexture, const GpuViewTextureSubresource& subresource)
		{
			std::optional<GpuView> newRTV = Allocate(EGpuViewType::RenderTargetView);
			if (newRTV)
			{
				check(newRTV->Type == EGpuViewType::RenderTargetView);
				check(newRTV->Index != InvalidIndexU32);
				UpdateRenderTargetView(*newRTV, gpuTexture, subresource);
			}
			else
			{
				checkNoEntry();
			}

			return MakeHandle(*newRTV);
		}

		auto RequestDepthStencilView(GpuTexture& gpuTexture, const GpuViewTextureSubresource& subresource)
		{
			std::optional<GpuView> newDSV = Allocate(EGpuViewType::DepthStencilView);
			if (newDSV)
			{
				check(newDSV->Type == EGpuViewType::DepthStencilView);
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
		Handle<GpuView, GpuViewManager*> MakeHandle(const GpuView& view);

		std::optional<GpuView> Allocate(const EGpuViewType type);
		void				   Deallocate(const GpuView& gpuView);

		void UpdateConstantBufferView(const GpuView& gpuView, GpuBuffer& buffer);
		void UpdateConstantBufferView(const GpuView& gpuView, GpuBuffer& buffer, const uint64_t offset, const uint64_t sizeInBytes);
		void UpdateShaderResourceView(const GpuView& gpuView, GpuBuffer& buffer);
		void UpdateUnorderedAccessView(const GpuView& gpuView, GpuBuffer& buffer);

		void UpdateShaderResourceView(const GpuView& gpuView, GpuTexture& gpuTexture, const GpuViewTextureSubresource& subresource);
		void UpdateUnorderedAccessView(const GpuView& gpuView, GpuTexture& gpuTexture, const GpuViewTextureSubresource& subresource);
		void UpdateRenderTargetView(const GpuView& gpuView, GpuTexture& gpuTexture, const GpuViewTextureSubresource& subresource);
		void UpdateDepthStencilView(const GpuView& gpuView, GpuTexture& gpuTexture, const GpuViewTextureSubresource& subresource);
		void operator()(details::HandleImpl handle, const uint64_t evaluatedTypeHash, GpuView* view);

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