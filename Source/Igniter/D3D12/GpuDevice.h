#pragma once
#include "Igniter/D3D12/Common.h"

namespace ig
{
    class CommandQueue;
    class CommandContext;
    class DescriptorHeap;
    class GpuView;
    class GpuBufferDesc;
    class GpuBuffer;
    class GpuTextureDesc;
    class GpuTexture;
    class GraphicsPipelineStateDesc;
    class ComputePipelineStateDesc;
    class PipelineState;
    class RootSignature;

    class GpuDevice final
    {
    public:
        GpuDevice();
        ~GpuDevice();

        GpuDevice(const GpuDevice&) = delete;
        GpuDevice(GpuDevice&&) noexcept = delete;

        GpuDevice& operator=(const GpuDevice&) = delete;
        GpuDevice& operator=(GpuDevice&&) noexcept = delete;

        [[nodiscard]] auto& GetNative() { return *device.Get(); }
        uint32_t GetDescriptorHandleIncrementSize(const EDescriptorHeapType type) const;

        Option<CommandQueue> CreateCommandQueue(const std::string_view debugName, const EQueueType queueType);
        Option<CommandContext> CreateCommandContext(const std::string_view debugName, const EQueueType targetQueueType);

        Option<RootSignature> CreateBindlessRootSignature();
        Option<PipelineState> CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc);
        Option<PipelineState> CreateComputePipelineState(const ComputePipelineStateDesc& desc);

        Option<GpuBuffer> CreateBuffer(const GpuBufferDesc& bufferDesc);
        Option<GpuTexture> CreateTexture(const GpuTextureDesc& textureDesc);

        Option<DescriptorHeap> CreateDescriptorHeap(
            const std::string_view debugName, const EDescriptorHeapType descriptorHeapType, const uint32_t numDescriptors);

        void CreateSampler(const D3D12_SAMPLER_DESC& samplerDesc, const GpuView& gpuView);

        void UpdateConstantBufferView(const GpuView& gpuView, GpuBuffer& buffer);
        void UpdateConstantBufferView(const GpuView& gpuView, GpuBuffer& buffer, const uint64_t offset, const uint64_t sizeInBytes);
        void UpdateShaderResourceView(const GpuView& gpuView, GpuBuffer& buffer);
        void UpdateUnorderedAccessView(const GpuView& gpuView, GpuBuffer& buffer);

        void UpdateShaderResourceView(const GpuView& gpuView, GpuTexture& texture, const GpuTextureSrvDesc& srvDesc, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
        void UpdateUnorderedAccessView(const GpuView& gpuView, GpuTexture& texture, const GpuTextureUavDesc& uavDesc, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
        void UpdateRenderTargetView(const GpuView& gpuView, GpuTexture& texture, const GpuTextureRtvDesc& rtvDesc, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
        void UpdateDepthStencilView(const GpuView& gpuView, GpuTexture& texture, const GpuTextureDsvDesc& dsvDesc, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);

        ComPtr<D3D12MA::Pool> CreateCustomMemoryPool(const D3D12MA::POOL_DESC& desc);

        GpuCopyableFootprints GetCopyableFootprints(
            const D3D12_RESOURCE_DESC1& resDesc, const uint32_t firstSubresource, const uint32_t numSubresources, const uint64_t baseOffset);

    private:
        bool AcquireAdapterFromFactory();
        void LogAdapterInformations();
        bool CreateDevice();
        void SetSeverityLevel();
        void CheckSupportedFeatures();
        void CacheDescriptorHandleIncrementSize();
        bool CreateMemoryAllcator();

    private:
        ComPtr<IDXGIAdapter> adapter;
        ComPtr<ID3D12Device10> device;

        D3D12MA::Allocator* allocator = nullptr;

        uint32_t cbvSrvUavDescriptorHandleIncrementSize = 0;
        uint32_t samplerDescritorHandleIncrementSize = 0;
        uint32_t dsvDescriptorHandleIncrementSize = 0;
        uint32_t rtvDescriptorHandleIncrementSize = 0;

        bool bEnhancedBarriersSupported = false;
        bool bRaytracing10Supported = false;
        bool bRaytracing11Supported = false;
        bool bShaderModel66Supported = false;
    };
}    // namespace ig
