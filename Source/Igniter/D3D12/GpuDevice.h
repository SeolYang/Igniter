#pragma once
#include "Igniter/D3D12/Common.h"

namespace ig
{
    class CommandQueue;
    class CommandList;
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
    class GpuFence;
    class CommandSignature;
    class CommandSignatureDesc;

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
        [[nodiscard]] const auto& GetNative() const { return *device.Get(); }
        [[nodiscard]] U32 GetDescriptorHandleIncrementSize(const EDescriptorHeapType type) const;

        [[nodiscard]] Option<CommandQueue> CreateCommandQueue(const std::string_view debugName, const EQueueType queueType);
        [[nodiscard]] Option<CommandList> CreateCommandList(const std::string_view debugName, const EQueueType targetQueueType);

        [[nodiscard]] Option<RootSignature> CreateBindlessRootSignature();
        [[nodiscard]] Option<PipelineState> CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc);
        [[nodiscard]] Option<PipelineState> CreateComputePipelineState(const ComputePipelineStateDesc& desc);

        [[nodiscard]] Option<GpuBuffer> CreateBuffer(const GpuBufferDesc& bufferDesc);
        [[nodiscard]] Option<GpuTexture> CreateTexture(const GpuTextureDesc& textureDesc);

        [[nodiscard]] Option<DescriptorHeap> CreateDescriptorHeap(const std::string_view debugName, const EDescriptorHeapType descriptorHeapType, const uint32_t numDescriptors);

        [[nodiscard]] Option<GpuFence> CreateFence(const std::string_view debugName);

        [[nodiscard]] Option<CommandSignature> CreateCommandSignature(const std::string_view debugName, const CommandSignatureDesc& desc, const Option<Ref<RootSignature>> rootSignatureOpt);

        void CreateSampler(const D3D12_SAMPLER_DESC& samplerDesc, const GpuView& gpuView);

        void CreateConstantBufferView(const GpuView& gpuView, GpuBuffer& buffer);
        void CreateConstantBufferView(const GpuView& gpuView, GpuBuffer& buffer, const uint64_t offset, const uint64_t sizeInBytes);
        void CreateShaderResourceView(const GpuView& gpuView, GpuBuffer& buffer);
        void CreateUnorderedAccessView(const GpuView& gpuView, GpuBuffer& buffer);

        void CreateShaderResourceView(const GpuView& gpuView, GpuTexture& texture, const GpuTextureSrvDesc& srvDesc, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
        void CreateUnorderedAccessView(const GpuView& gpuView, GpuTexture& texture, const GpuTextureUavDesc& uavDesc, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
        void CreateRenderTargetView(const GpuView& gpuView, GpuTexture& texture, const GpuTextureRtvDesc& rtvDesc, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
        void CreateDepthStencilView(const GpuView& gpuView, GpuTexture& texture, const GpuTextureDsvDesc& dsvDesc, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);

        [[nodiscard]] ComPtr<D3D12MA::Pool> CreateCustomMemoryPool(const D3D12MA::POOL_DESC& desc);

        [[nodiscard]] GpuCopyableFootprints GetCopyableFootprints(const D3D12_RESOURCE_DESC1& resDesc, const uint32_t firstSubresource, const uint32_t numSubresources, const uint64_t baseOffset) const;

        [[nodiscard]] HRESULT GetDeviceRemovedReason() const { return device->GetDeviceRemovedReason(); }

    private:
        bool AcquireAdapterFromFactory();
        void LogAdapterInformation() const;
        bool CreateDevice();
        void SetSeverityLevel();
        void CheckSupportedFeatures();
        void CacheDescriptorHandleIncrementSize();
        bool CreateMemoryAllocator();

    private:
        ComPtr<IDXGIAdapter> adapter;
        ComPtr<ID3D12Device10> device;

        D3D12MA::Allocator* allocator = nullptr;

        U32 cbvSrvUavDescriptorHandleIncrementSize = 0;
        U32 samplerDescriptorHandleIncrementSize = 0;
        U32 dsvDescriptorHandleIncrementSize = 0;
        U32 rtvDescriptorHandleIncrementSize = 0;

        bool bEnhancedBarriersSupported = false;
        bool bRaytracing10Supported = false;
        bool bRaytracing11Supported = false;
        bool bShaderModel66Supported = false;
    };
} // namespace ig
