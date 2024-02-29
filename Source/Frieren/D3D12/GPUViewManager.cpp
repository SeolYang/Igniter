#include <D3D12/GpuViewManager.h>
#include <D3D12/GpuView.h>
#include <D3D12/RenderDevice.h>
#include <D3D12/DescriptorHeap.h>
#include <Core/Assert.h>
#include <Core/HandleManager.h>

namespace fe
{
	GpuViewManager::GpuViewManager(HandleManager& handleManager, DeferredDeallocator& deferredDeallocator, RenderDevice& device)
		: handleManager(handleManager),
		  deferredDeallocator(deferredDeallocator),
		  device(device),
		  cbvSrvUavHeap(std::make_unique<DescriptorHeap>(device.CreateDescriptorHeap(EDescriptorHeapType::CBV_SRV_UAV, NumCbvSrvUavDescriptors).value())),
		  samplerHeap(std::make_unique<DescriptorHeap>(device.CreateDescriptorHeap(EDescriptorHeapType::Sampler, NumSamplerDescriptors).value())),
		  rtvHeap(std::make_unique<DescriptorHeap>(device.CreateDescriptorHeap(EDescriptorHeapType::RTV, NumRtvDescriptors).value())),
		  dsvHeap(std::make_unique<DescriptorHeap>(device.CreateDescriptorHeap(EDescriptorHeapType::DSV, NumDsvDescriptors).value()))
	{
	}

	GpuViewManager::~GpuViewManager()
	{
	}

	std::array<std::reference_wrapper<DescriptorHeap>, 2> GpuViewManager::GetBindlessDescriptorHeaps()
	{
		return { *cbvSrvUavHeap, *samplerHeap };
	}

	void GpuViewManager::UpdateConstantBufferView(const GpuView& gpuView, GpuBuffer& buffer)
	{
		device.UpdateConstantBufferView(gpuView, buffer);
	}

	void GpuViewManager::UpdateConstantBufferView(const GpuView& gpuView, GpuBuffer& buffer, const uint64_t offset, const uint64_t sizeInBytes)
	{
		device.UpdateConstantBufferView(gpuView, buffer, offset, sizeInBytes);
	}

	void GpuViewManager::UpdateShaderResourceView(const GpuView& gpuView, GpuBuffer& buffer)
	{
		device.UpdateShaderResourceView(gpuView, buffer);
	}

	void GpuViewManager::UpdateUnorderedAccessView(const GpuView& gpuView, GpuBuffer& buffer)
	{
		device.UpdateUnorderedAccessView(gpuView, buffer);
	}

	void GpuViewManager::UpdateShaderResourceView(const GpuView& gpuView, GpuTexture& gpuTexture, const GpuViewTextureSubresource& subresource)
	{
		device.UpdateShaderResourceView(gpuView, gpuTexture, subresource);
	}

	void GpuViewManager::UpdateUnorderedAccessView(const GpuView& gpuView, GpuTexture& gpuTexture, const GpuViewTextureSubresource& subresource)
	{
		device.UpdateUnorderedAccessView(gpuView, gpuTexture, subresource);
	}

	void GpuViewManager::UpdateRenderTargetView(const GpuView& gpuView, GpuTexture& gpuTexture, const GpuViewTextureSubresource& subresource)
	{
		device.UpdateRenderTargetView(gpuView, gpuTexture, subresource);
	}

	void GpuViewManager::UpdateDepthStencilView(const GpuView& gpuView, GpuTexture& gpuTexture, const GpuViewTextureSubresource& subresource)
	{
		device.UpdateDepthStencilView(gpuView, gpuTexture, subresource);
	}

	Handle<GpuView, GpuViewManager*> GpuViewManager::MakeHandle(const GpuView& view)
	{
		Handle<GpuView, GpuViewManager*> res;
		if (view.IsValid())
		{
			res = Handle<GpuView, GpuViewManager*>{
				handleManager, this, view
			};
		}
		
		return res;
	}
	
	std::optional<GpuView> GpuViewManager::Allocate(const EGpuViewType type)
	{
		check(cbvSrvUavHeap != nullptr && samplerHeap != nullptr && rtvHeap != nullptr && dsvHeap != nullptr);
		switch (type)
		{
			case EGpuViewType::ConstantBufferView:
				return cbvSrvUavHeap->Allocate(type);
			case EGpuViewType::ShaderResourceView:
				return cbvSrvUavHeap->Allocate(type);
			case EGpuViewType::UnorderedAccessView:
				return cbvSrvUavHeap->Allocate(type);
			case EGpuViewType::Sampler:
				return samplerHeap->Allocate(type);
			case EGpuViewType::RenderTargetView:
				return rtvHeap->Allocate(type);
			case EGpuViewType::DepthStencilView:
				return dsvHeap->Allocate(type);
		}

		checkNoEntry();
		return {};
	}

	void GpuViewManager::Deallocate(const GpuView& gpuView)
	{
		check(gpuView.IsValid());
		check(cbvSrvUavHeap != nullptr && samplerHeap != nullptr && rtvHeap != nullptr && dsvHeap != nullptr);
		switch (gpuView.Type)
		{
			case EGpuViewType::ConstantBufferView:
				return cbvSrvUavHeap->Deallocate(gpuView);
			case EGpuViewType::ShaderResourceView:
				return cbvSrvUavHeap->Deallocate(gpuView);
			case EGpuViewType::UnorderedAccessView:
				return cbvSrvUavHeap->Deallocate(gpuView);
			case EGpuViewType::Sampler:
				return samplerHeap->Deallocate(gpuView);
			case EGpuViewType::RenderTargetView:
				return rtvHeap->Deallocate(gpuView);
			case EGpuViewType::DepthStencilView:
				return dsvHeap->Deallocate(gpuView);
		}

		checkNoEntry();
	}

	void GpuViewManager::operator()(details::HandleImpl handle, const uint64_t evaluatedTypeHash, GpuView* view)
	{
		RequestDeferredDeallocation(deferredDeallocator, [this, handle, evaluatedTypeHash, view]() {
			check(view != nullptr && view->IsValid());
			this->Deallocate(*view);
			details::HandleImpl targetHandle = handle;
			targetHandle.Deallocate(evaluatedTypeHash);
		});
	}
} // namespace fe