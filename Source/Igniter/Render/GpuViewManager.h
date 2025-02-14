#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/Handle.h"
#include "Igniter/D3D12/Common.h"
#include "Igniter/D3D12/GpuView.h"

namespace ig
{
    class GpuDevice;
    class DescriptorHeap;
    class GpuTexture;
    class GpuBuffer;

    class GpuViewManager final
    {
    public:
        GpuViewManager(GpuDevice& gpuDevice,
                       const U32 numCbvSrvUavDescriptors = DefaultNumCbvSrvUavDescriptors,
                       const U32 numSamplerDescriptors = DefaultNumSamplerDescriptors,
                       const U32 numRtvDescriptors = DefaultNumRtvDescriptors,
                       const U32 numDsvDescriptors = DefaultNumDsvDescriptors);
        GpuViewManager(const GpuViewManager&) = delete;
        GpuViewManager(GpuViewManager&&) noexcept = delete;
        ~GpuViewManager();

        GpuViewManager& operator=(const GpuViewManager&) = delete;
        GpuViewManager& operator=(GpuViewManager&&) noexcept = delete;

        GpuView Allocate(const EGpuViewType type);

        GpuView RequestConstantBufferView(GpuBuffer& gpuBuffer);
        GpuView RequestConstantBufferView(GpuBuffer& gpuBuffer, const uint64_t offset, const uint64_t sizeInBytes);
        GpuView RequestShaderResourceView(GpuBuffer& gpuBuffer);
        GpuView RequestUnorderedAccessView(GpuBuffer& gpuBuffer);

        GpuView RequestShaderResourceView(GpuTexture& gpuTexture, const GpuTextureSrvDesc& srvDesc, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
        GpuView RequestUnorderedAccessView(GpuTexture& gpuTexture, const GpuTextureUavDesc& uavDesc, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
        GpuView RequestRenderTargetView(GpuTexture& gpuTexture, const GpuTextureRtvDesc& rtvDesc, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
        GpuView RequestDepthStencilView(GpuTexture& gpuTexture, const GpuTextureDsvDesc& dsvDesc, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
        GpuView RequestSampler(const D3D12_SAMPLER_DESC& desc);

        void Deallocate(const GpuView& gpuView);

        DescriptorHeap& GetCbvSrvUavDescHeap();
        DescriptorHeap& GetSamplerDescHeap();
        DescriptorHeap& GetRtvDescHeap();
        DescriptorHeap& GetDsvDescHeap();

    private:
        constexpr static U32 DefaultNumCbvSrvUavDescriptors = 4096;
        constexpr static U32 DefaultNumSamplerDescriptors = 64;
        constexpr static U32 DefaultNumRtvDescriptors = 512;
        constexpr static U32 DefaultNumDsvDescriptors = DefaultNumRtvDescriptors;

        GpuDevice& gpuDevice;
        Ptr<DescriptorHeap> cbvSrvUavHeap;
        Ptr<DescriptorHeap> samplerHeap;
        Ptr<DescriptorHeap> rtvHeap;
        Ptr<DescriptorHeap> dsvHeap;

        UnorderedMap<uint64_t, GpuView> cachedSamplerView;
    };
} // namespace ig
