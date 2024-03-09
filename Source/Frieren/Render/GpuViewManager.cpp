#include <Render/GpuViewManager.h>
#include <D3D12/GpuView.h>
#include <D3D12/RenderDevice.h>
#include <D3D12/DescriptorHeap.h>
#include <Core/Assert.h>
#include <Core/HandleManager.h>
#include <Core/DeferredDeallocator.h>

namespace fe
{
	GpuViewManager::GpuViewManager(HandleManager& handleManager, DeferredDeallocator& deferredDeallocator, RenderDevice& device)
		: handleManager(handleManager),
		  deferredDeallocator(deferredDeallocator),
		  device(device),
		  cbvSrvUavHeap(std::make_unique<DescriptorHeap>(device.CreateDescriptorHeap("Bindless CBV-SRV-UAV Heap", EDescriptorHeapType::CBV_SRV_UAV, NumCbvSrvUavDescriptors).value())),
		  samplerHeap(std::make_unique<DescriptorHeap>(device.CreateDescriptorHeap("Bindless Sampler Heap", EDescriptorHeapType::Sampler, NumSamplerDescriptors).value())),
		  rtvHeap(std::make_unique<DescriptorHeap>(device.CreateDescriptorHeap("Bindless RTV Heap", EDescriptorHeapType::RTV, NumRtvDescriptors).value())),
		  dsvHeap(std::make_unique<DescriptorHeap>(device.CreateDescriptorHeap("Bindless DSV Heap", EDescriptorHeapType::DSV, NumDsvDescriptors).value()))
	{
	}

	GpuViewManager::~GpuViewManager()
	{
	}

	std::array<std::reference_wrapper<DescriptorHeap>, 2> GpuViewManager::GetBindlessDescriptorHeaps()
	{
		return { *cbvSrvUavHeap, *samplerHeap };
	}

	Handle<GpuView, GpuViewManager*> GpuViewManager::RequestConstantBufferView(GpuBuffer& gpuBuffer)
	{
		std::optional<GpuView> newCBV = Allocate(EGpuViewType::ConstantBufferView);
		if (newCBV)
		{
			check(newCBV->Type == EGpuViewType::ConstantBufferView);
			check(newCBV->Index != InvalidIndexU32);
			device.UpdateConstantBufferView(*newCBV, gpuBuffer);
			return MakeHandle(*newCBV);
		}

		checkNoEntry();
		return {};
	}

	Handle<GpuView, GpuViewManager*> GpuViewManager::RequestConstantBufferView(GpuBuffer& gpuBuffer, const uint64_t offset, const uint64_t sizeInBytes)
	{
		std::optional<GpuView> newCBV = Allocate(EGpuViewType::ConstantBufferView);
		if (newCBV)
		{
			check(newCBV->Type == EGpuViewType::ConstantBufferView);
			check(newCBV->Index != InvalidIndexU32);
			check(offset % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0);
			check(sizeInBytes % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0);
			device.UpdateConstantBufferView(*newCBV, gpuBuffer, offset, sizeInBytes);
			return MakeHandle(*newCBV);
		}

		checkNoEntry();
		return {};
	}

	Handle<GpuView, GpuViewManager*> GpuViewManager::RequestShaderResourceView(GpuBuffer& gpuBuffer)
	{
		std::optional<GpuView> newSRV = Allocate(EGpuViewType::ShaderResourceView);
		if (newSRV)
		{
			check(newSRV->Type == EGpuViewType::ShaderResourceView);
			check(newSRV->Index != InvalidIndexU32);
			device.UpdateShaderResourceView(*newSRV, gpuBuffer);
			return MakeHandle(*newSRV);
		}

		checkNoEntry();
		return {};
	}
	
	Handle<GpuView, GpuViewManager*> GpuViewManager::RequestUnorderedAccessView(GpuBuffer& gpuBuffer)
	{
		std::optional<GpuView> newUAV = Allocate(EGpuViewType::UnorderedAccessView);
		if (newUAV)
		{
			check(newUAV->Type == EGpuViewType::UnorderedAccessView);
			check(newUAV->Index != InvalidIndexU32);
			device.UpdateUnorderedAccessView(*newUAV, gpuBuffer);
			return MakeHandle(*newUAV);
		}

		checkNoEntry();
		return {};
	}

	Handle<GpuView, GpuViewManager*> GpuViewManager::RequestShaderResourceView(GpuTexture& gpuTexture, const GpuViewTextureSubresource& subresource)
	{
		std::optional<GpuView> newSRV = Allocate(EGpuViewType::ShaderResourceView);
		if (newSRV)
		{
			check(newSRV->Type == EGpuViewType::ShaderResourceView);
			check(newSRV->Index != InvalidIndexU32);
			device.UpdateShaderResourceView(*newSRV, gpuTexture, subresource);
			return MakeHandle(*newSRV);
		}

		checkNoEntry();
		return {};
	}

	Handle<GpuView, GpuViewManager*> GpuViewManager::RequestUnorderedAccessView(GpuTexture& gpuTexture, const GpuViewTextureSubresource& subresource)
	{
		std::optional<GpuView> newUAV = Allocate(EGpuViewType::UnorderedAccessView);
		if (newUAV)
		{
			check(newUAV->Type == EGpuViewType::UnorderedAccessView);
			check(newUAV->Index != InvalidIndexU32);
			device.UpdateUnorderedAccessView(*newUAV, gpuTexture, subresource);
			return MakeHandle(*newUAV);
		}

		checkNoEntry();
		return {};
	}

	Handle<GpuView, GpuViewManager*> GpuViewManager::RequestRenderTargetView(GpuTexture& gpuTexture, const GpuViewTextureSubresource& subresource)
	{
		std::optional<GpuView> newRTV = Allocate(EGpuViewType::RenderTargetView);
		if (newRTV)
		{
			check(newRTV->Type == EGpuViewType::RenderTargetView);
			check(newRTV->Index != InvalidIndexU32);
			device.UpdateRenderTargetView(*newRTV, gpuTexture, subresource);
			return MakeHandle(*newRTV);
		}

		checkNoEntry();
		return {};
	}

	Handle<GpuView, GpuViewManager*> GpuViewManager::RequestDepthStencilView(GpuTexture& gpuTexture, const GpuViewTextureSubresource& subresource)
	{
		std::optional<GpuView> newDSV = Allocate(EGpuViewType::DepthStencilView);
		if (newDSV)
		{
			check(newDSV->Type == EGpuViewType::DepthStencilView);
			check(newDSV->Index != InvalidIndexU32);
			device.UpdateDepthStencilView(*newDSV, gpuTexture, subresource);
			return MakeHandle(*newDSV);
		}

		checkNoEntry();
		return {};
	}

	Handle<GpuView, GpuViewManager*> GpuViewManager::MakeHandle(const GpuView& view)
	{
		Handle<GpuView, GpuViewManager*> res;
		if (view.IsValid())
		{
			res = Handle<GpuView, GpuViewManager*>{ handleManager, this, view };
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
		deferredDeallocator.RequestDeallocation(
			[this, handle, evaluatedTypeHash, view]()
			{
				check(view != nullptr && view->IsValid());
				this->Deallocate(*view);
				details::HandleImpl targetHandle = handle;
				targetHandle.Deallocate(evaluatedTypeHash);
			});
	}
} // namespace fe