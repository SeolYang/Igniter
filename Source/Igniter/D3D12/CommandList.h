#pragma once
#include "Igniter/D3D12/Common.h"

namespace ig
{
    class GpuDevice;
    class DescriptorHeap;
    class GpuTexture;
    class GpuBuffer;
    class GpuView;
    class RootSignature;
    class CommandSignature;
    class PipelineState;

    class CommandList final
    {
        friend class GpuDevice;

    public:
        using NativeType = ID3D12GraphicsCommandList7;

    public:
        CommandList(const CommandList&) = delete;
        CommandList(CommandList&& other) noexcept;
        ~CommandList() = default;

        CommandList& operator=(const CommandList&) = delete;
        CommandList& operator=(CommandList&& other) noexcept;

        auto& GetNative() { return *cmdList.Get(); }

        bool IsValid() const { return cmdAllocator && cmdList; }
        operator bool() const { return IsValid(); }
        bool IsReadyToExecute() const { return pendingGlobalBarriers.empty() && pendingTextureBarriers.empty() && pendingBufferBarriers.empty(); }

        // 기록된 커맨드들은 다시 Open을 호출하지 않는 이상 그대로 유지된다.
        void Open(PipelineState* const initState = nullptr);
        void Close();

        // https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html#equivalent-d3d12_barrier_sync-bit-for-each-d3d12_resource_states-bit
        void AddPendingTextureBarrier(GpuTexture& targetTexture,
                                      const D3D12_BARRIER_SYNC syncBefore, const D3D12_BARRIER_SYNC syncAfter,
                                      D3D12_BARRIER_ACCESS accessBefore, const D3D12_BARRIER_ACCESS accessAfter,
                                      const D3D12_BARRIER_LAYOUT layoutBefore, const D3D12_BARRIER_LAYOUT layoutAfter,
                                      const D3D12_BARRIER_SUBRESOURCE_RANGE subresourceRange = {0xffffffff, 0, 0, 0, 0, 0});
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
        void CopyBuffer(GpuBuffer& src, const size_t srcOffsetInBytes, const size_t numBytes,
                        GpuBuffer& dst, const size_t dstOffsetInBytes);
        void CopyTextureRegion(GpuBuffer& src, const size_t srcOffsetInBytes,
                               GpuTexture& dst, const uint32_t subresourceIdx,
                               const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& layout);
        void CopyTextureSimple(GpuTexture& src, GpuTexture& dst);

        void SetRootSignature(RootSignature& rootSignature);
        void SetDescriptorHeaps(const std::span<DescriptorHeap*> descriptorHeaps);
        void SetDescriptorHeap(DescriptorHeap& descriptorHeap);
        void SetVertexBuffer(GpuBuffer& vertexBuffer);
        void SetIndexBuffer(GpuBuffer& indexBuffer);
        void SetRenderTarget(const GpuView& rtv, std::optional<std::reference_wrapper<GpuView>> dsv = std::nullopt);
        void SetPrimitiveTopology(const D3D12_PRIMITIVE_TOPOLOGY primitiveTopology);
        void SetViewport(const float topLeftX, const float topLeftY, const float width, const float height, const float minDepth = 0.f,
                         const float maxDepth = 1.f);
        void SetViewport(const Viewport& viewport);
        void SetScissorRect(const long left, const long top, const long right, const long bottom);
        void SetScissorRect(const Viewport& viewport);
        void SetRoot32BitConstants(const uint32_t registerSlot, const uint32_t num32BitValuesToSet,
                                   const void* srcData, const uint32_t destOffsetIn32BitValues);
        template <typename T>
            requires(std::is_pod_v<T>)
        void SetRoot32BitConstants(const uint32_t registerSlot, const T& data, const uint32_t destOffsetIn32BitValues)
        {
            IG_CHECK(sizeof(T) % 4 == 0 && "Data loss may occur specific conditions.");
            SetRoot32BitConstants(registerSlot, sizeof(T) / 4, reinterpret_cast<const void*>(&data), destOffsetIn32BitValues);
        }

        void DrawIndexed(const uint32_t numIndices, const uint32_t indexOffset = 0, const uint32_t vertexOffset = 0);

        void Dispatch(U32 threadGroupSizeX, U32 threadGroupSizeY, U32 threadGroupSizeZ);

        void ExecuteIndirect(CommandSignature& cmdSignature, GpuBuffer& cmdBuffer);

    private:
        CommandList(ComPtr<ID3D12CommandAllocator> newCmdAllocator, ComPtr<NativeType> newCmdList, const EQueueType targetQueueType);

    private:
        ComPtr<ID3D12CommandAllocator> cmdAllocator;
        ComPtr<NativeType> cmdList;
        EQueueType cmdListTargetQueueType;

        Vector<D3D12_GLOBAL_BARRIER> pendingGlobalBarriers;
        Vector<D3D12_TEXTURE_BARRIER> pendingTextureBarriers;
        Vector<D3D12_BUFFER_BARRIER> pendingBufferBarriers;
    };
} // namespace ig