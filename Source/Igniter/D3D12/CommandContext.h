#pragma once
#include <D3D12/Common.h>
#include <Core/String.h>
#include <Core/Container.h>
#include <Math/Common.h>

namespace ig
{
    class RenderDevice;
    class DescriptorHeap;
    class GpuTexture;
    class GpuBuffer;
    class GpuView;
    class RootSignature;
    class PipelineState;
    class CommandContext
    {
        friend class RenderDevice;

    public:
        using NativeType = ID3D12GraphicsCommandList7;

    public:
        CommandContext(const CommandContext&) = delete;
        CommandContext(CommandContext&& other) noexcept;
        ~CommandContext() = default;

        CommandContext& operator=(const CommandContext&) = delete;
        CommandContext& operator=(CommandContext&& other) noexcept;

        auto& GetNative() { return *cmdList.Get(); }

        bool IsValid() const { return cmdAllocator && cmdList; }
        operator bool() const { return IsValid(); }

        bool IsReadyToExecute() const
        {
            return pendingGlobalBarriers.empty() && pendingTextureBarriers.empty() && pendingBufferBarriers.empty();
        }

        // Equivalent to Reset
        void Begin(PipelineState* const initState = nullptr);
        // Equivalent to Close
        void End();

        // https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html#equivalent-d3d12_barrier_sync-bit-for-each-d3d12_resource_states-bit
        void AddPendingTextureBarrier(GpuTexture& targetTexture,
                                      const D3D12_BARRIER_SYNC syncBefore, const D3D12_BARRIER_SYNC syncAfter,
                                      D3D12_BARRIER_ACCESS accessBefore, const D3D12_BARRIER_ACCESS accessAfter,
                                      const D3D12_BARRIER_LAYOUT layoutBefore, const D3D12_BARRIER_LAYOUT layoutAfter,
                                      const D3D12_BARRIER_SUBRESOURCE_RANGE subresourceRange = { 0xffffffff, 0, 0, 0, 0,
                                                                                                 0 });
        void AddPendingBufferBarrier(GpuBuffer& targetBuffer,
                                     const D3D12_BARRIER_SYNC syncBefore, const D3D12_BARRIER_SYNC syncAfter,
                                     D3D12_BARRIER_ACCESS accessBefore, const D3D12_BARRIER_ACCESS accessAfter,
                                     const size_t offset = 0, const size_t sizeAsBytes = std::numeric_limits<size_t>::max());
        void FlushBarriers();

        void ClearRenderTarget(const GpuView& rtv, float r = 0.f, float g = 0.f, float b = 0.f, float a = 1.f);
        void ClearDepthStencil(const GpuView& dsv, float depth = 1.f, uint8_t stencil = 0);
        void ClearDepth(const GpuView& dsv, float depth = 1.f);
        void ClearStencil(const GpuView& dsv, uint8_t stencil = 0);

        void CopyBuffer(GpuBuffer& src, GpuBuffer& dst);
        void CopyBuffer(GpuBuffer& src, const size_t srcOffsetInBytes, const size_t numBytes, GpuBuffer& dst, const size_t dstOffsetInBytes);
        void CopyTextureRegion(GpuBuffer& src, const size_t srcOffsetInBytes, GpuTexture& dst, const uint32_t subresourceIdx, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& layout);

        void SetRootSignature(RootSignature& rootSignature);
        void SetDescriptorHeaps(const std::span<std::reference_wrapper<DescriptorHeap>> targetDescriptorHeaps);
        void SetDescriptorHeap(DescriptorHeap& descriptorHeap);
        void SetVertexBuffer(GpuBuffer& vertexBuffer);
        void SetIndexBuffer(GpuBuffer& indexBuffer);
        void SetRenderTarget(const GpuView& rtv, std::optional<std::reference_wrapper<GpuView>> dsv = std::nullopt);
        void SetPrimitiveTopology(const D3D12_PRIMITIVE_TOPOLOGY primitiveTopology);
        void SetViewport(const float topLeftX, const float topLeftY, const float width, const float height, const float minDepth = 0.f, const float maxDepth = 1.f);
        void SetViewport(const Viewport& viewport);
        void SetScissorRect(const long left, const long top, const long right, const long bottom);
        void SetScissorRect(const Viewport& viewport);

        void SetRoot32BitConstants(const uint32_t registerSlot, const uint32_t num32BitValuesToSet, const void* srcData, const uint32_t destOffsetIn32BitValues);

        template <typename T>
            requires(std::is_pod_v<T>)
        void SetRoot32BitConstants(const uint32_t registerSlot, const T& data, const uint32_t destOffsetIn32BitValues)
        {
            check(sizeof(T) % 4 == 0 && "Data loss may occur specific conditions.");
            SetRoot32BitConstants(registerSlot, sizeof(T) / 4, reinterpret_cast<const void*>(&data), destOffsetIn32BitValues);
        }

        void DrawIndexed(const uint32_t numIndices, const uint32_t indexOffset = 0, const uint32_t vertexOffset = 0);

    private:
        CommandContext(ComPtr<ID3D12CommandAllocator> newCmdAllocator, ComPtr<NativeType> newCmdList,
                       const EQueueType targetQueueType);

    private:
        ComPtr<ID3D12CommandAllocator> cmdAllocator;
        ComPtr<NativeType> cmdList;
        EQueueType cmdListTargetQueueType;

        std::vector<D3D12_GLOBAL_BARRIER> pendingGlobalBarriers;
        std::vector<D3D12_TEXTURE_BARRIER> pendingTextureBarriers;
        std::vector<D3D12_BUFFER_BARRIER> pendingBufferBarriers;
    };
} // namespace ig