#include <Igniter.h>
#include <Core/Hash.h>
#include <D3D12/RenderDevice.h>
#include <D3D12/DescriptorHeap.h>
#include <Render/GpuViewManager.h>

namespace ig
{
    GpuViewManager::GpuViewManager(RenderDevice& renderDevice)
        : renderDevice(renderDevice)
        , cbvSrvUavHeap(MakePtr<DescriptorHeap>(
              renderDevice.CreateDescriptorHeap("Bindless CBV-SRV-UAV Heap", EDescriptorHeapType::CBV_SRV_UAV, NumCbvSrvUavDescriptors).value()))
        , samplerHeap(MakePtr<DescriptorHeap>(
              renderDevice.CreateDescriptorHeap("Bindless Sampler Heap", EDescriptorHeapType::Sampler, NumSamplerDescriptors).value()))
        , rtvHeap(
              MakePtr<DescriptorHeap>(renderDevice.CreateDescriptorHeap("Bindless RTV Heap", EDescriptorHeapType::RTV, NumRtvDescriptors).value()))
        , dsvHeap(
              MakePtr<DescriptorHeap>(renderDevice.CreateDescriptorHeap("Bindless DSV Heap", EDescriptorHeapType::DSV, NumDsvDescriptors).value()))
    {
    }

    GpuViewManager::~GpuViewManager()
    {
        for (const auto& [_, samplerView] : cachedSamplerView)
        {
            samplerHeap->Deallocate(samplerView);
        }
    }

    GpuView GpuViewManager::RequestConstantBufferView(GpuBuffer& gpuBuffer)
    {
        const GpuView newCBV = Allocate(EGpuViewType::ConstantBufferView);
        if (newCBV)
        {
            IG_CHECK(newCBV.Type == EGpuViewType::ConstantBufferView);
            renderDevice.UpdateConstantBufferView(newCBV, gpuBuffer);
        }

        return newCBV;
    }

    GpuView GpuViewManager::RequestConstantBufferView(GpuBuffer& gpuBuffer, const uint64_t offset, const uint64_t sizeInBytes)
    {
        IG_CHECK(sizeInBytes > 0);
        IG_CHECK(offset % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0);
        IG_CHECK(sizeInBytes % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0);

        const GpuView newCBV = Allocate(EGpuViewType::ConstantBufferView);
        if (newCBV)
        {
            IG_CHECK(newCBV.Type == EGpuViewType::ConstantBufferView);
            renderDevice.UpdateConstantBufferView(newCBV, gpuBuffer, offset, sizeInBytes);
        }

        return newCBV;
    }

    GpuView GpuViewManager::RequestShaderResourceView(GpuBuffer& gpuBuffer)
    {
        const GpuView newSRV = Allocate(EGpuViewType::ShaderResourceView);
        if (newSRV)
        {
            IG_CHECK(newSRV.Type == EGpuViewType::ShaderResourceView);
            renderDevice.UpdateShaderResourceView(newSRV, gpuBuffer);
        }

        return newSRV;
    }

    GpuView GpuViewManager::RequestUnorderedAccessView(GpuBuffer& gpuBuffer)
    {
        const GpuView newUAV = Allocate(EGpuViewType::UnorderedAccessView);
        if (newUAV)
        {
            IG_CHECK(newUAV.Type == EGpuViewType::UnorderedAccessView);
            renderDevice.UpdateUnorderedAccessView(newUAV, gpuBuffer);
        }

        return newUAV;
    }

    GpuView GpuViewManager::RequestShaderResourceView(
        GpuTexture& gpuTexture, const GpuTextureSrvDesc& srvDesc, const DXGI_FORMAT desireViewFormat /*= DXGI_FORMAT_UNKNOWN*/)
    {
        const GpuView newSRV = Allocate(EGpuViewType::ShaderResourceView);
        if (newSRV)
        {
            IG_CHECK(newSRV.Type == EGpuViewType::ShaderResourceView);
            renderDevice.UpdateShaderResourceView(newSRV, gpuTexture, srvDesc, desireViewFormat);
        }

        return newSRV;
    }

    GpuView GpuViewManager::RequestUnorderedAccessView(
        GpuTexture& gpuTexture, const GpuTextureUavDesc& uavDesc, const DXGI_FORMAT desireViewFormat /*= DXGI_FORMAT_UNKNOWN*/)
    {
        const GpuView newUAV = Allocate(EGpuViewType::UnorderedAccessView);
        if (newUAV)
        {
            IG_CHECK(newUAV.Type == EGpuViewType::UnorderedAccessView);
            renderDevice.UpdateUnorderedAccessView(newUAV, gpuTexture, uavDesc, desireViewFormat);
        }

        return newUAV;
    }

    GpuView GpuViewManager::RequestRenderTargetView(
        GpuTexture& gpuTexture, const GpuTextureRtvDesc& rtvDesc, const DXGI_FORMAT desireViewFormat /*= DXGI_FORMAT_UNKNOWN*/)
    {
        const GpuView newRTV = Allocate(EGpuViewType::RenderTargetView);
        if (newRTV)
        {
            IG_CHECK(newRTV.Type == EGpuViewType::RenderTargetView);
            renderDevice.UpdateRenderTargetView(newRTV, gpuTexture, rtvDesc, desireViewFormat);
        }

        return newRTV;
    }

    GpuView GpuViewManager::RequestDepthStencilView(
        GpuTexture& gpuTexture, const GpuTextureDsvDesc& dsvDesc, const DXGI_FORMAT desireViewFormat /*= DXGI_FORMAT_UNKNOWN*/)
    {
        const GpuView newDSV = Allocate(EGpuViewType::DepthStencilView);
        if (newDSV)
        {
            IG_CHECK(newDSV.Type == EGpuViewType::DepthStencilView);
            renderDevice.UpdateDepthStencilView(newDSV, gpuTexture, dsvDesc, desireViewFormat);
        }

        return newDSV;
    }

    GpuView GpuViewManager::RequestSampler(const D3D12_SAMPLER_DESC& desc)
    {
        const uint64_t samplerDescHashVal = HashState(&desc);

        const auto itr = cachedSamplerView.find(samplerDescHashVal);
        if (itr != cachedSamplerView.end())
        {
            return itr->second;
        }

        GpuView samplerView = Allocate(EGpuViewType::Sampler);
        if (samplerView)
        {
            IG_CHECK(samplerView.Type == EGpuViewType::Sampler);
            renderDevice.CreateSampler(desc, samplerView);
            cachedSamplerView[samplerDescHashVal] = samplerView;
        }

        return samplerView;
    }

    GpuView GpuViewManager::Allocate(const EGpuViewType type)
    {
        IG_CHECK(cbvSrvUavHeap != nullptr && samplerHeap != nullptr && rtvHeap != nullptr && dsvHeap != nullptr);

        /* #sy_todo DescriptorHeap.h:30 해결 후 바로 Allocate 반환 하는 방식으로 변경 할 것 */
        std::optional<GpuView> allocatedView{};
        switch (type)
        {
            case EGpuViewType::ConstantBufferView:
                allocatedView = cbvSrvUavHeap->Allocate(type);
                break;
            case EGpuViewType::ShaderResourceView:
                allocatedView = cbvSrvUavHeap->Allocate(type);
                break;
            case EGpuViewType::UnorderedAccessView:
                allocatedView = cbvSrvUavHeap->Allocate(type);
                break;
            case EGpuViewType::Sampler:
                allocatedView = samplerHeap->Allocate(type);
                break;
            case EGpuViewType::RenderTargetView:
                allocatedView = rtvHeap->Allocate(type);
                break;
            case EGpuViewType::DepthStencilView:
                allocatedView = dsvHeap->Allocate(type);
                break;
        }

        return allocatedView ? *allocatedView : GpuView{};
    }

    void GpuViewManager::Deallocate(const GpuView& gpuView)
    {
        IG_CHECK(gpuView.IsValid());
        switch (gpuView.Type)
        {
            case EGpuViewType::ConstantBufferView:
                cbvSrvUavHeap->Deallocate(gpuView);
                break;
            case EGpuViewType::ShaderResourceView:
                cbvSrvUavHeap->Deallocate(gpuView);
                break;
            case EGpuViewType::UnorderedAccessView:
                cbvSrvUavHeap->Deallocate(gpuView);
                break;
            case EGpuViewType::RenderTargetView:
                rtvHeap->Deallocate(gpuView);
                break;
            case EGpuViewType::DepthStencilView:
                dsvHeap->Deallocate(gpuView);
                break;
            case EGpuViewType::Sampler:
                /* 샘플러 뷰는 캐시 되어 있기 때문에 해제하지 않음 */
                break;
            default:
                IG_CHECK_NO_ENTRY();
                break;
        }
    }

    DescriptorHeap& GpuViewManager::GetCbvSrvUavDescHeap()
    {
        return *cbvSrvUavHeap;
    }

    DescriptorHeap& GpuViewManager::GetSamplerDescHeap()
    {
        return *samplerHeap;
    }

    DescriptorHeap& GpuViewManager::GetRtvDescHeap()
    {
        return *rtvHeap;
    }

    DescriptorHeap& GpuViewManager::GetDsvDescHeap()
    {
        return *dsvHeap;
    }
}    // namespace ig