#include "Igniter/Igniter.h"
#include "Igniter/Core/Hash.h"
#include "Igniter/D3D12/GpuDevice.h"
#include "Igniter/D3D12/DescriptorHeap.h"
#include "Igniter/Render/GpuViewManager.h"

namespace ig
{
    GpuViewManager::GpuViewManager(GpuDevice& gpuDevice, const U32 numCbvSrvUavDescriptors, const U32 numSamplerDescriptors, const U32 numRtvDescriptors, const U32 numDsvDescriptors)
        : gpuDevice(gpuDevice)
        , cbvSrvUavHeap(MakePtr<DescriptorHeap>(gpuDevice.CreateDescriptorHeap("GpuViewManagerCbvSrvUavHeap", EDescriptorHeapType::CBV_SRV_UAV, numCbvSrvUavDescriptors).value()))
        , samplerHeap(MakePtr<DescriptorHeap>(gpuDevice.CreateDescriptorHeap("GpuViewManagerSamplerHeap", EDescriptorHeapType::Sampler, numSamplerDescriptors).value()))
        , rtvHeap(MakePtr<DescriptorHeap>(gpuDevice.CreateDescriptorHeap("GpuViewManagerRtvHeap", EDescriptorHeapType::RTV, numRtvDescriptors).value()))
        , dsvHeap(MakePtr<DescriptorHeap>(gpuDevice.CreateDescriptorHeap("GpuViewManagerDsvHeap", EDescriptorHeapType::DSV, numDsvDescriptors).value()))
    {
    }

    GpuViewManager::~GpuViewManager()
    {
        for (const auto& [_, samplerView] : cachedSamplerView)
        {
            gpuDevice.DestroySampler(samplerView);
            samplerHeap->Deallocate(samplerView);
        }
    }

    GpuView GpuViewManager::RequestConstantBufferView(GpuBuffer& gpuBuffer)
    {
        const GpuView newCBV = Allocate(EGpuViewType::ConstantBufferView);
        if (newCBV)
        {
            IG_CHECK(newCBV.Type == EGpuViewType::ConstantBufferView);
            gpuDevice.CreateConstantBufferView(newCBV, gpuBuffer);
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
            gpuDevice.CreateConstantBufferView(newCBV, gpuBuffer, offset, sizeInBytes);
        }

        return newCBV;
    }

    GpuView GpuViewManager::RequestShaderResourceView(GpuBuffer& gpuBuffer)
    {
        const GpuView newSRV = Allocate(EGpuViewType::ShaderResourceView);
        if (newSRV)
        {
            IG_CHECK(newSRV.Type == EGpuViewType::ShaderResourceView);
            gpuDevice.CreateShaderResourceView(newSRV, gpuBuffer);
        }

        return newSRV;
    }

    GpuView GpuViewManager::RequestUnorderedAccessView(GpuBuffer& gpuBuffer)
    {
        const GpuView newUAV = Allocate(EGpuViewType::UnorderedAccessView);
        if (newUAV)
        {
            IG_CHECK(newUAV.Type == EGpuViewType::UnorderedAccessView);
            gpuDevice.CreateUnorderedAccessView(newUAV, gpuBuffer);
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
            gpuDevice.CreateShaderResourceView(newSRV, gpuTexture, srvDesc, desireViewFormat);
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
            gpuDevice.CreateUnorderedAccessView(newUAV, gpuTexture, uavDesc, desireViewFormat);
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
            gpuDevice.CreateRenderTargetView(newRTV, gpuTexture, rtvDesc, desireViewFormat);
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
            gpuDevice.CreateDepthStencilView(newDSV, gpuTexture, dsvDesc, desireViewFormat);
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
            gpuDevice.CreateSampler(desc, samplerView);
            cachedSamplerView[samplerDescHashVal] = samplerView;
        }

        return samplerView;
    }

    GpuView GpuViewManager::Allocate(const EGpuViewType type)
    {
        IG_CHECK(cbvSrvUavHeap != nullptr && samplerHeap != nullptr && rtvHeap != nullptr && dsvHeap != nullptr);

        switch (type)
        {
        case EGpuViewType::ConstantBufferView:
        case EGpuViewType::ShaderResourceView:
        case EGpuViewType::UnorderedAccessView:
            return cbvSrvUavHeap->Allocate(type);

        case EGpuViewType::Sampler:
            return samplerHeap->Allocate(type);

        case EGpuViewType::RenderTargetView:
            return rtvHeap->Allocate(type);

        case EGpuViewType::DepthStencilView:
            return dsvHeap->Allocate(type);

        default:
            return GpuView{};
        }
    }

    void GpuViewManager::Deallocate(const GpuView& gpuView)
    {
        IG_CHECK(gpuView.IsValid());
        switch (gpuView.Type)
        {
        case EGpuViewType::ConstantBufferView:
            gpuDevice.DestroyConstantBufferView(gpuView);
            cbvSrvUavHeap->Deallocate(gpuView);
            break;
        case EGpuViewType::ShaderResourceView:
            gpuDevice.DestroyShaderResourceView(gpuView);
            cbvSrvUavHeap->Deallocate(gpuView);
            break;
        case EGpuViewType::UnorderedAccessView:
            gpuDevice.DestroyUnorderedAccessView(gpuView);
            cbvSrvUavHeap->Deallocate(gpuView);
            break;
        case EGpuViewType::RenderTargetView:
            gpuDevice.DestroyRenderTargetView(gpuView);
            rtvHeap->Deallocate(gpuView);
            break;
        case EGpuViewType::DepthStencilView:
            gpuDevice.DestroyDepthStencilView(gpuView);
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
} // namespace ig
