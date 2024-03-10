#pragma once
#include <D3D12/Common.h>
#include <D3D12/GPUView.h>
#include <Core/Handle.h>

namespace fe
{
	class DeferredDeallocator;
	class HandleManager;
	class GpuBuffer;
	class GpuTexture;
	class RenderDevice;
	class DescriptorHeap;
	class GpuViewManager
	{
		friend class Handle<GpuView, GpuViewManager*>;

	public:
		GpuViewManager(HandleManager& handleManager, DeferredDeallocator& deferredDeallocator, RenderDevice& device);
		GpuViewManager(const GpuViewManager&) = delete;
		GpuViewManager(GpuViewManager&&) noexcept = delete;
		~GpuViewManager();

		GpuViewManager& operator=(const GpuViewManager&) = delete;
		GpuViewManager& operator=(GpuViewManager&&) noexcept = delete;

		std::array<std::reference_wrapper<DescriptorHeap>, 2> GetBindlessDescriptorHeaps();

		Handle<GpuView, GpuViewManager*> RequestConstantBufferView(GpuBuffer& gpuBuffer);
		Handle<GpuView, GpuViewManager*> RequestConstantBufferView(GpuBuffer& gpuBuffer, const uint64_t offset, const uint64_t sizeInBytes);
		Handle<GpuView, GpuViewManager*> RequestShaderResourceView(GpuBuffer& gpuBuffer);
		Handle<GpuView, GpuViewManager*> RequestUnorderedAccessView(GpuBuffer& gpuBuffer);

		Handle<GpuView, GpuViewManager*> RequestShaderResourceView(GpuTexture& gpuTexture, const GpuTextureSrvDesc& srvDesc, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
		Handle<GpuView, GpuViewManager*> RequestUnorderedAccessView(GpuTexture& gpuTexture, const GpuTextureUavDesc& uavDesc, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
		Handle<GpuView, GpuViewManager*> RequestRenderTargetView(GpuTexture& gpuTexture, const GpuTextureRtvDesc& rtvDesc, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
		Handle<GpuView, GpuViewManager*> RequestDepthStencilView(GpuTexture& gpuTexture, const GpuTextureDsvDesc& dsvDesc, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);

	private:
		Handle<GpuView, GpuViewManager*> MakeHandle(const GpuView& view);

		std::optional<GpuView> Allocate(const EGpuViewType type);
		void Deallocate(const GpuView& gpuView);

		void operator()(details::HandleImpl handle, const uint64_t evaluatedTypeHash, GpuView* view);

	private:
		HandleManager& handleManager;
		DeferredDeallocator& deferredDeallocator;
		RenderDevice& device;

		std::unique_ptr<DescriptorHeap> cbvSrvUavHeap;
		std::unique_ptr<DescriptorHeap> samplerHeap;
		std::unique_ptr<DescriptorHeap> rtvHeap;
		std::unique_ptr<DescriptorHeap> dsvHeap;

		static constexpr uint32_t NumCbvSrvUavDescriptors = 4096;
		static constexpr uint32_t NumSamplerDescriptors = 256;
		static constexpr uint32_t NumRtvDescriptors = 512;
		static constexpr uint32_t NumDsvDescriptors = NumRtvDescriptors;
	};
} // namespace fe