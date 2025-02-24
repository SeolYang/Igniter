#pragma once
#include "Igniter/Core/String.h"
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
    class MeshShaderPipelineStateDesc;
    class PipelineState;
    class RootSignature;
    class GpuFence;
    class CommandSignature;
    class CommandSignatureDesc;

    class GpuDevice final
    {
    public:
        struct Statistics
        {
            String DeviceName{};
            bool bIsUma; // false == discrete gpu, true == integrated gpu

            Size DedicatedVideoMemUsage{0};
            Size DedicatedVideoMemBudget{0};
            Size DedicatedVideoMemBlockCount{0};
            Size DedicatedVideoMemBlockSize{0};
            Size DedicatedVideoMemAllocCount{0};
            Size DedicatedVideoMemAllocSize{0};

            Size SharedVideoMemUsage{0};
            Size SharedVideoMemBudget{0};
            Size SharedVideoMemBlockCount{0};
            Size SharedVideoMemBlockSize{0};
            Size SharedVideoMemAllocCount{0};
            Size SharedVideoMemAllocSize{0};
        };

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
        [[nodiscard]] GpuCopyableFootprints GetCopyableFootprints(const D3D12_RESOURCE_DESC1& resDesc, const U32 firstSubresource, const U32 numSubresources, const uint64_t baseOffset) const;
        [[nodiscard]] HRESULT GetDeviceRemovedReason() const { return device->GetDeviceRemovedReason(); }
        [[nodiscard]] Statistics GetStatistics() const;
        [[nodiscard]] bool IsGpuUploadHeapSupported() const noexcept { return bGpuUploadHeapSupported; }

        [[nodiscard]] Option<CommandQueue> CreateCommandQueue(const std::string_view debugName, const EQueueType queueType);
        [[nodiscard]] Option<CommandList> CreateCommandList(const std::string_view debugName, const EQueueType targetQueueType);
        [[nodiscard]] Option<RootSignature> CreateBindlessRootSignature();
        [[nodiscard]] Option<PipelineState> CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc);
        [[nodiscard]] Option<PipelineState> CreateComputePipelineState(const ComputePipelineStateDesc& desc);
        [[nodiscard]] Option<PipelineState> CreateMeshShaderPipelineState(const MeshShaderPipelineStateDesc& desc);
        [[nodiscard]] Option<GpuBuffer> CreateBuffer(const GpuBufferDesc& bufferDesc);
        [[nodiscard]] Option<GpuTexture> CreateTexture(const GpuTextureDesc& textureDesc);
        [[nodiscard]] Option<DescriptorHeap> CreateDescriptorHeap(const std::string_view debugName, const EDescriptorHeapType descriptorHeapType, const U32 numDescriptors);
        [[nodiscard]] Option<GpuFence> CreateFence(const std::string_view debugName);
        [[nodiscard]] Option<CommandSignature> CreateCommandSignature(const std::string_view debugName, const CommandSignatureDesc& desc, const Option<Ref<RootSignature>> rootSignatureOpt);
        [[nodiscard]] ComPtr<D3D12MA::Pool> CreateCustomMemoryPool(const D3D12MA::POOL_DESC& desc);

        void CreateSampler(const D3D12_SAMPLER_DESC& samplerDesc, const GpuView& gpuView);
        void DestroySampler(const GpuView& gpuView);

        void CreateConstantBufferView(const GpuView& gpuView, GpuBuffer& buffer);
        void CreateConstantBufferView(const GpuView& gpuView, GpuBuffer& buffer, const uint64_t offset, const uint64_t sizeInBytes);
        void CreateShaderResourceView(const GpuView& gpuView, GpuBuffer& buffer, const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc);
        VOID CreateUnorderedAccessView(const GpuView& gpuView, GpuBuffer& buffer, const D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc);
        void CreateShaderResourceView(const GpuView& gpuView, GpuTexture& texture, const GpuTextureSrvVariant& srvVariant, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
        void CreateUnorderedAccessView(const GpuView& gpuView, GpuTexture& texture, const GpuTextureUavVariant& uavVariant, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
        void CreateRenderTargetView(const GpuView& gpuView, GpuTexture& texture, const GpuTextureRtvVariant& rtvVariant, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
        void CreateDepthStencilView(const GpuView& gpuView, GpuTexture& texture, const GpuTextureDsvVariant& dsvVariant, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);

        void DestroyConstantBufferView(const GpuView& gpuView);
        void DestroyShaderResourceView(const GpuView& gpuView);
        void DestroyUnorderedAccessView(const GpuView& gpuView);
        void DestroyRenderTargetView(const GpuView& gpuView);
        void DestroyDepthStencilView(const GpuView& gpuView);

    private:
        bool AcquireAdapterFromFactory();
        void AcquireAdapterInfo();
        bool CreateDevice();
        void SetSeverityLevel();
        void CheckSupportedFeatures();
        void CacheDescriptorHandleIncrementSize();
        bool CreateMemoryAllocator();

    private:
        ComPtr<IDXGIAdapter> adapter;
        String name{};

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
        bool bGpuUploadHeapSupported = false;
    };
} // namespace ig
