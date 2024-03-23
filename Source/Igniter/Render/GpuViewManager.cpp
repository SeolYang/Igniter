#include <Igniter.h>
#include <Render/GpuViewManager.h>
#include <D3D12/GpuView.h>
#include <D3D12/RenderDevice.h>
#include <D3D12/DescriptorHeap.h>
#include <Core/Assert.h>
#include <Core/HandleManager.h>
#include <Core/DeferredDeallocator.h>
#include <Core/HashUtils.h>

namespace ig
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
            IG_CHECK(newCBV->Type == EGpuViewType::ConstantBufferView);
            IG_CHECK(newCBV->Index != InvalidIndexU32);
            device.UpdateConstantBufferView(*newCBV, gpuBuffer);
            return MakeHandle(*newCBV);
        }

        IG_CHECK_NO_ENTRY();
        return {};
    }

    Handle<GpuView, GpuViewManager*> GpuViewManager::RequestConstantBufferView(GpuBuffer& gpuBuffer, const uint64_t offset, const uint64_t sizeInBytes)
    {
        std::optional<GpuView> newCBV = Allocate(EGpuViewType::ConstantBufferView);
        if (newCBV)
        {
            IG_CHECK(newCBV->Type == EGpuViewType::ConstantBufferView);
            IG_CHECK(newCBV->Index != InvalidIndexU32);
            IG_CHECK(offset % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0);
            IG_CHECK(sizeInBytes % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0);
            device.UpdateConstantBufferView(*newCBV, gpuBuffer, offset, sizeInBytes);
            return MakeHandle(*newCBV);
        }

        IG_CHECK_NO_ENTRY();
        return {};
    }

    Handle<GpuView, GpuViewManager*> GpuViewManager::RequestShaderResourceView(GpuBuffer& gpuBuffer)
    {
        std::optional<GpuView> newSRV = Allocate(EGpuViewType::ShaderResourceView);
        if (newSRV)
        {
            IG_CHECK(newSRV->Type == EGpuViewType::ShaderResourceView);
            IG_CHECK(newSRV->Index != InvalidIndexU32);
            device.UpdateShaderResourceView(*newSRV, gpuBuffer);
            return MakeHandle(*newSRV);
        }

        IG_CHECK_NO_ENTRY();
        return {};
    }

    Handle<GpuView, GpuViewManager*> GpuViewManager::RequestUnorderedAccessView(GpuBuffer& gpuBuffer)
    {
        std::optional<GpuView> newUAV = Allocate(EGpuViewType::UnorderedAccessView);
        if (newUAV)
        {
            IG_CHECK(newUAV->Type == EGpuViewType::UnorderedAccessView);
            IG_CHECK(newUAV->Index != InvalidIndexU32);
            device.UpdateUnorderedAccessView(*newUAV, gpuBuffer);
            return MakeHandle(*newUAV);
        }

        IG_CHECK_NO_ENTRY();
        return {};
    }

    Handle<GpuView, GpuViewManager*> GpuViewManager::RequestShaderResourceView(GpuTexture& gpuTexture, const GpuTextureSrvDesc& srvDesc, const DXGI_FORMAT desireViewFormat /*= DXGI_FORMAT_UNKNOWN*/)
    {
        std::optional<GpuView> newSRV = Allocate(EGpuViewType::ShaderResourceView);
        if (newSRV)
        {
            IG_CHECK(newSRV->Type == EGpuViewType::ShaderResourceView);
            IG_CHECK(newSRV->Index != InvalidIndexU32);
            device.UpdateShaderResourceView(*newSRV, gpuTexture, srvDesc, desireViewFormat);
            return MakeHandle(*newSRV);
        }

        IG_CHECK_NO_ENTRY();
        return {};
    }

    Handle<GpuView, GpuViewManager*> GpuViewManager::RequestUnorderedAccessView(GpuTexture& gpuTexture, const GpuTextureUavDesc& uavDesc, const DXGI_FORMAT desireViewFormat /*= DXGI_FORMAT_UNKNOWN*/)
    {
        std::optional<GpuView> newUAV = Allocate(EGpuViewType::UnorderedAccessView);
        if (newUAV)
        {
            IG_CHECK(newUAV->Type == EGpuViewType::UnorderedAccessView);
            IG_CHECK(newUAV->Index != InvalidIndexU32);
            device.UpdateUnorderedAccessView(*newUAV, gpuTexture, uavDesc, desireViewFormat);
            return MakeHandle(*newUAV);
        }

        IG_CHECK_NO_ENTRY();
        return {};
    }

    Handle<GpuView, GpuViewManager*> GpuViewManager::RequestRenderTargetView(GpuTexture& gpuTexture, const GpuTextureRtvDesc& rtvDesc, const DXGI_FORMAT desireViewFormat /*= DXGI_FORMAT_UNKNOWN*/)
    {
        std::optional<GpuView> newRTV = Allocate(EGpuViewType::RenderTargetView);
        if (newRTV)
        {
            IG_CHECK(newRTV->Type == EGpuViewType::RenderTargetView);
            IG_CHECK(newRTV->Index != InvalidIndexU32);
            device.UpdateRenderTargetView(*newRTV, gpuTexture, rtvDesc, desireViewFormat);
            return MakeHandle(*newRTV);
        }

        IG_CHECK_NO_ENTRY();
        return {};
    }

    Handle<GpuView, GpuViewManager*> GpuViewManager::RequestDepthStencilView(GpuTexture& gpuTexture, const GpuTextureDsvDesc& dsvDesc, const DXGI_FORMAT desireViewFormat /*= DXGI_FORMAT_UNKNOWN*/)
    {
        std::optional<GpuView> newDSV = Allocate(EGpuViewType::DepthStencilView);
        if (newDSV)
        {
            IG_CHECK(newDSV->Type == EGpuViewType::DepthStencilView);
            IG_CHECK(newDSV->Index != InvalidIndexU32);
            device.UpdateDepthStencilView(*newDSV, gpuTexture, dsvDesc, desireViewFormat);
            return MakeHandle(*newDSV);
        }

        IG_CHECK_NO_ENTRY();
        return {};
    }

    RefHandle<GpuView> GpuViewManager::RequestSampler(const D3D12_SAMPLER_DESC& desc)
    {
        RecursiveLock lock{ mutex };
        const uint64_t samplerDescHashVal = HashState(&desc);

        RefHandle<GpuView> refHandle{};
        const auto itr = cachedSamplerView.find(samplerDescHashVal);
        if (itr != cachedSamplerView.end())
        {
            refHandle = itr->second.MakeRef();
        }
        else
        {
            std::optional<GpuView> samplerView = samplerHeap->Allocate(EGpuViewType::Sampler);
            if (!samplerView)
            {
                IG_CHECK_NO_ENTRY();
            }
            else
            {
                IG_CHECK(samplerView->Type == EGpuViewType::Sampler);
                IG_CHECK(samplerView->HasValidCPUHandle());
                device.CreateSampler(desc, *samplerView);

                Handle<GpuView, GpuViewManager*> handle = MakeHandle(*samplerView);
                refHandle = handle.MakeRef();
                cachedSamplerView[samplerDescHashVal] = std::move(handle);
            }
        }

        IG_CHECK(refHandle);
        return refHandle;
    }

    void GpuViewManager::ClearCachedSampler()
    {
        cachedSamplerView.clear();
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
        RecursiveLock lock{ mutex };
        IG_CHECK(cbvSrvUavHeap != nullptr && samplerHeap != nullptr && rtvHeap != nullptr && dsvHeap != nullptr);
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

        IG_CHECK_NO_ENTRY();
        return {};
    }

    void GpuViewManager::Deallocate(const GpuView& gpuView)
    {
        RecursiveLock lock{ mutex };
        IG_CHECK(gpuView.IsValid());
        IG_CHECK(cbvSrvUavHeap != nullptr && samplerHeap != nullptr && rtvHeap != nullptr && dsvHeap != nullptr);
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

        IG_CHECK_NO_ENTRY();
    }

    void GpuViewManager::operator()(details::HandleImpl handle, const uint64_t evaluatedTypeHash, GpuView* view)
    {
        deferredDeallocator.RequestDeallocation(
            [this, handle, evaluatedTypeHash, view]()
            {
                IG_CHECK(view != nullptr && view->IsValid());
                this->Deallocate(*view);
                details::HandleImpl targetHandle = handle;
                targetHandle.Deallocate(evaluatedTypeHash);
            });
    }
} // namespace ig