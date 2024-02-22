#include <D3D12/GPUViewManager.h>
#include <D3D12/GPUView.h>
#include <D3D12/Device.h>
#include <D3D12/DescriptorHeap.h>
#include <Core/Assert.h>
#include <Core/HandleManager.h>

namespace fe::dx
{
	GPUViewManager::GPUViewManager(HandleManager& handleManager, DeferredDeallocator& deferredDeallocator, Device& device)
		: handleManager(handleManager),
		  deferredDeallocator(deferredDeallocator),
		  device(device),
		  cbvSrvUavHeap(std::make_unique<DescriptorHeap>(device.CreateDescriptorHeap(EDescriptorHeapType::CBV_SRV_UAV, NumCbvSrvUavDescriptors).value())),
		  samplerHeap(std::make_unique<DescriptorHeap>(device.CreateDescriptorHeap(EDescriptorHeapType::Sampler, NumSamplerDescriptors).value())),
		  rtvHeap(std::make_unique<DescriptorHeap>(device.CreateDescriptorHeap(EDescriptorHeapType::RTV, NumRtvDescriptors).value())),
		  dsvHeap(std::make_unique<DescriptorHeap>(device.CreateDescriptorHeap(EDescriptorHeapType::DSV, NumDsvDescriptors).value()))
	{
	}

	GPUViewManager::~GPUViewManager()
	{
	}

	std::array<std::reference_wrapper<DescriptorHeap>, 2> GPUViewManager::GetBindlessDescriptorHeaps()
	{
		return { *cbvSrvUavHeap, *samplerHeap };
	}

	void GPUViewManager::UpdateConstantBufferView(const GPUView& gpuView, GPUBuffer& buffer)
	{
		device.UpdateConstantBufferView(gpuView, buffer);
	}

	void GPUViewManager::UpdateShaderResourceView(const GPUView& gpuView, GPUBuffer& buffer)
	{
		device.UpdateShaderResourceView(gpuView, buffer);
	}

	void GPUViewManager::UpdateUnorderedAccessView(const GPUView& gpuView, GPUBuffer& buffer)
	{
		device.UpdateUnorderedAccessView(gpuView, buffer);
	}

	void GPUViewManager::UpdateShaderResourceView(const GPUView& gpuView, GPUTexture& gpuTexture, const GPUTextureSubresource& subresource)
	{
		device.UpdateShaderResourceView(gpuView, gpuTexture, subresource);
	}

	void GPUViewManager::UpdateUnorderedAccessView(const GPUView& gpuView, GPUTexture& gpuTexture, const GPUTextureSubresource& subresource)
	{
		device.UpdateUnorderedAccessView(gpuView, gpuTexture, subresource);
	}

	void GPUViewManager::UpdateRenderTargetView(const GPUView& gpuView, GPUTexture& gpuTexture, const GPUTextureSubresource& subresource)
	{
		device.UpdateRenderTargetView(gpuView, gpuTexture, subresource);
	}

	void GPUViewManager::UpdateDepthStencilView(const GPUView& gpuView, GPUTexture& gpuTexture, const GPUTextureSubresource& subresource)
	{
		device.UpdateDepthStencilView(gpuView, gpuTexture, subresource);
	}

	UniqueHandle<GPUView, GPUViewManager*> GPUViewManager::MakeHandle(const GPUView& view)
	{
		UniqueHandle<GPUView, GPUViewManager*> res;
		if (view.IsValid())
		{
			res = UniqueHandle<GPUView, GPUViewManager*>{
				handleManager, this, view
			};
		}
		
		return res;
	}
	
	std::optional<GPUView> GPUViewManager::Allocate(const EGPUViewType type)
	{
		check(cbvSrvUavHeap != nullptr && samplerHeap != nullptr && rtvHeap != nullptr && dsvHeap != nullptr);
		switch (type)
		{
			case EGPUViewType::ConstantBufferView:
				return cbvSrvUavHeap->Allocate(type);
			case EGPUViewType::ShaderResourceView:
				return cbvSrvUavHeap->Allocate(type);
			case EGPUViewType::UnorderedAccessView:
				return cbvSrvUavHeap->Allocate(type);
			case EGPUViewType::Sampler:
				return samplerHeap->Allocate(type);
			case EGPUViewType::RenderTargetView:
				return rtvHeap->Allocate(type);
			case EGPUViewType::DepthStencilView:
				return dsvHeap->Allocate(type);
		}

		checkNoEntry();
		return {};
	}

	void GPUViewManager::Deallocate(const GPUView& gpuView)
	{
		check(gpuView.IsValid());
		check(cbvSrvUavHeap != nullptr && samplerHeap != nullptr && rtvHeap != nullptr && dsvHeap != nullptr);
		switch (gpuView.Type)
		{
			case EGPUViewType::ConstantBufferView:
				return cbvSrvUavHeap->Deallocate(gpuView);
			case EGPUViewType::ShaderResourceView:
				return cbvSrvUavHeap->Deallocate(gpuView);
			case EGPUViewType::UnorderedAccessView:
				return cbvSrvUavHeap->Deallocate(gpuView);
			case EGPUViewType::Sampler:
				return samplerHeap->Deallocate(gpuView);
			case EGPUViewType::RenderTargetView:
				return rtvHeap->Deallocate(gpuView);
			case EGPUViewType::DepthStencilView:
				return dsvHeap->Deallocate(gpuView);
		}

		checkNoEntry();
	}

	void GPUViewManager::operator()(Handle handle, const uint64_t evaluatedTypeHash)
	{
		RequestDeferredDeallocation(deferredDeallocator, [this, handle, evaluatedTypeHash]() {
			const GPUView* view = reinterpret_cast<const GPUView*>(handle.GetAddressOf(evaluatedTypeHash));
			check(view != nullptr && view->IsValid());
			this->Deallocate(*view);
			Handle targetHandle = handle;
			targetHandle.Deallocate(evaluatedTypeHash);
		});
	}
} // namespace fe::dx