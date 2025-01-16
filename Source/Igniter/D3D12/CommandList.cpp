#include "Igniter/Igniter.h"
#include "Igniter/Core/ContainerUtils.h"
#include "Igniter/D3D12/CommandList.h"
#include "Igniter/D3D12/GpuDevice.h"
#include "Igniter/D3D12/GPUTextureDesc.h"
#include "Igniter/D3D12/GPUTexture.h"
#include "Igniter/D3D12/GPUBufferDesc.h"
#include "Igniter/D3D12/GPUBuffer.h"
#include "Igniter/D3D12/GPUView.h"
#include "Igniter/D3D12/PipelineState.h"
#include "Igniter/D3D12/DescriptorHeap.h"
#include "Igniter/D3D12/RootSignature.h"
#include "Igniter/D3D12/CommandSignature.h"

namespace ig
{
    CommandList::CommandList(CommandList&& other) noexcept
        : cmdAllocator(std::move(other.cmdAllocator))
        , cmdList(std::move(other.cmdList))
        , cmdListTargetQueueType(other.cmdListTargetQueueType)
        , pendingGlobalBarriers(std::move(other.pendingGlobalBarriers))
        , pendingTextureBarriers(std::move(other.pendingTextureBarriers))
        , pendingBufferBarriers(std::move(other.pendingBufferBarriers))
    {
    }

    CommandList::CommandList(ComPtr<ID3D12CommandAllocator> newCmdAllocator, ComPtr<NativeType> newCmdList, const EQueueType targetQueueType)
        : cmdAllocator(std::move(newCmdAllocator))
        , cmdList(std::move(newCmdList))
        , cmdListTargetQueueType(targetQueueType)
    {
    }

    CommandList& CommandList::operator=(CommandList&& other) noexcept
    {
        cmdAllocator = std::move(other.cmdAllocator);
        cmdList = std::move(other.cmdList);
        cmdListTargetQueueType = other.cmdListTargetQueueType;
        pendingGlobalBarriers = std::move(other.pendingGlobalBarriers);
        pendingTextureBarriers = std::move(other.pendingTextureBarriers);
        pendingBufferBarriers = std::move(other.pendingBufferBarriers);
        return *this;
    }

    void CommandList::Open(PipelineState* const initStatePtr)
    {
        IG_VERIFY(cmdAllocator.Get() != nullptr);
        IG_VERIFY(cmdList.Get() != nullptr);

        auto* const nativeInitStatePtr = initStatePtr != nullptr ? &initStatePtr->GetNative() : nullptr;
        IG_VERIFY(initStatePtr == nullptr || (initStatePtr != nullptr && nativeInitStatePtr != nullptr));
        cmdAllocator->Reset();
        cmdList->Reset(cmdAllocator.Get(), nativeInitStatePtr);
    }

    void CommandList::Close()
    {
        IG_CHECK(pendingGlobalBarriers.empty());
        IG_CHECK(pendingTextureBarriers.empty());
        IG_CHECK(pendingBufferBarriers.empty());
        IG_VERIFY(cmdList.Get() != nullptr);
        IG_VERIFY_SUCCEEDED(cmdList->Close());
    }

    void CommandList::AddPendingTextureBarrier(GpuTexture& targetTexture,
                                               const D3D12_BARRIER_SYNC syncBefore, const D3D12_BARRIER_SYNC syncAfter,
                                               D3D12_BARRIER_ACCESS accessBefore, const D3D12_BARRIER_ACCESS accessAfter,
                                               const D3D12_BARRIER_LAYOUT layoutBefore, const D3D12_BARRIER_LAYOUT layoutAfter,
                                               const D3D12_BARRIER_SUBRESOURCE_RANGE subresourceRange)
    {
        IG_CHECK(IsValid());
        IG_CHECK(targetTexture);
        IG_CHECK(syncBefore != syncAfter || accessBefore != accessAfter || layoutBefore != layoutAfter);

        pendingTextureBarriers.emplace_back(
            D3D12_TEXTURE_BARRIER{
                .SyncBefore = syncBefore,
                .SyncAfter = syncAfter,
                .AccessBefore = accessBefore,
                .AccessAfter = accessAfter,
                .LayoutBefore = layoutBefore,
                .LayoutAfter = layoutAfter,
                .pResource = &targetTexture.GetNative(),
                .Subresources = subresourceRange,
                .Flags = D3D12_TEXTURE_BARRIER_FLAG_NONE});
    }

    void CommandList::AddPendingBufferBarrier(GpuBuffer& targetBuffer,
                                              const D3D12_BARRIER_SYNC syncBefore, const D3D12_BARRIER_SYNC syncAfter,
                                              D3D12_BARRIER_ACCESS accessBefore, const D3D12_BARRIER_ACCESS accessAfter,
                                              const size_t offset, const size_t sizeAsBytes)
    {
        IG_CHECK(IsValid());
        IG_CHECK(targetBuffer);
        IG_CHECK(syncBefore != syncAfter || accessBefore != accessAfter);

        pendingBufferBarriers.emplace_back(
            D3D12_BUFFER_BARRIER{
                .SyncBefore = syncBefore,
                .SyncAfter = syncAfter,
                .AccessBefore = accessBefore,
                .AccessAfter = accessAfter,
                .pResource = &targetBuffer.GetNative(),
                .Offset = offset,
                .Size = sizeAsBytes});
    }

    void CommandList::FlushBarriers()
    {
        IG_CHECK(IsValid());
        eastl::vector<D3D12_BARRIER_GROUP> barrierGroups{};
        barrierGroups.reserve(3);

        if (!pendingGlobalBarriers.empty())
        {
            barrierGroups.emplace_back(
                D3D12_BARRIER_GROUP{
                    .Type = D3D12_BARRIER_TYPE_GLOBAL,
                    .NumBarriers = static_cast<U32>(pendingGlobalBarriers.size()),
                    .pGlobalBarriers = pendingGlobalBarriers.data()});
        }

        if (!pendingTextureBarriers.empty())
        {
            barrierGroups.emplace_back(
                D3D12_BARRIER_GROUP{
                    .Type = D3D12_BARRIER_TYPE_TEXTURE,
                    .NumBarriers = static_cast<U32>(pendingTextureBarriers.size()),
                    .pTextureBarriers = pendingTextureBarriers.data()});
        }

        if (!pendingBufferBarriers.empty())
        {
            barrierGroups.emplace_back(
                D3D12_BARRIER_GROUP{
                    .Type = D3D12_BARRIER_TYPE_BUFFER,
                    .NumBarriers = static_cast<U32>(pendingBufferBarriers.size()),
                    .pBufferBarriers = pendingBufferBarriers.data()});
        }

        cmdList->Barrier((UINT32)barrierGroups.size(), barrierGroups.data());
        pendingGlobalBarriers.clear();
        pendingTextureBarriers.clear();
        pendingBufferBarriers.clear();
    }

    void CommandList::ClearRenderTarget(const GpuView& rtv, F32 r /*= 0.f*/, F32 g /*= 0.f*/, F32 b /*= 0.f*/, F32 a /*= 1.f*/)
    {
        IG_CHECK(IsValid());
        IG_CHECK(cmdListTargetQueueType == EQueueType::Graphics);
        IG_CHECK(rtv && (rtv.Type == EGpuViewType::RenderTargetView));
        const F32 rgba[4] = {r, g, b, a};
        cmdList->ClearRenderTargetView(rtv.CpuHandle, rgba, 0, nullptr);
    }

    void CommandList::ClearDepthStencil(const GpuView& dsv, F32 depth /*= 1.f*/, uint8_t stencil /*= 0*/)
    {
        IG_CHECK(IsValid());
        IG_CHECK(cmdListTargetQueueType == EQueueType::Graphics);
        IG_CHECK(dsv && (dsv.Type == EGpuViewType::DepthStencilView));

        const D3D12_CPU_DESCRIPTOR_HANDLE dsvCpuHandle = dsv.CpuHandle;
        cmdList->ClearDepthStencilView(dsvCpuHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, 0, nullptr);
    }

    void CommandList::ClearDepth(const GpuView& dsv, F32 depth /*= 1.f*/)
    {
        IG_CHECK(IsValid());
        IG_CHECK(cmdListTargetQueueType == EQueueType::Graphics);
        IG_CHECK(dsv && (dsv.Type == EGpuViewType::DepthStencilView));

        const D3D12_CPU_DESCRIPTOR_HANDLE dsvCpuHandle = dsv.CpuHandle;
        cmdList->ClearDepthStencilView(dsvCpuHandle, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
    }

    void CommandList::ClearStencil(const GpuView& dsv, uint8_t stencil /*= 0*/)
    {
        IG_CHECK(IsValid());
        IG_CHECK(cmdListTargetQueueType == EQueueType::Graphics);
        IG_CHECK(dsv && (dsv.Type == EGpuViewType::DepthStencilView));

        const D3D12_CPU_DESCRIPTOR_HANDLE dsvCpuHandle = dsv.CpuHandle;
        cmdList->ClearDepthStencilView(dsvCpuHandle, D3D12_CLEAR_FLAG_STENCIL, 0.f, stencil, 0, nullptr);
    }

    void CommandList::CopyBuffer(GpuBuffer& src,
                                 const size_t srcOffsetInBytes, const size_t numBytes,
                                 GpuBuffer& dst, const size_t dstOffsetInBytes)
    {
        IG_CHECK(IsValid());
        IG_CHECK(src);
        IG_CHECK(dst);
        IG_CHECK(numBytes > 0);
        IG_CHECK(dstOffsetInBytes + numBytes <= dst.GetDesc().GetSizeAsBytes());
        IG_CHECK(srcOffsetInBytes + numBytes <= src.GetDesc().GetSizeAsBytes());

        cmdList->CopyBufferRegion(&dst.GetNative(), dstOffsetInBytes, &src.GetNative(), srcOffsetInBytes, numBytes);
    }

    void CommandList::CopyBuffer(GpuBuffer& src, GpuBuffer& dst)
    {
        const auto& srcDesc = src.GetDesc();
        CopyBuffer(src, 0, srcDesc.GetSizeAsBytes(), dst, 0);
    }

    void CommandList::CopyTextureRegion(GpuBuffer& src, const size_t srcOffsetInBytes, GpuTexture& dst, const U32 subresourceIdx,
                                        const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& layout)
    {
        IG_CHECK(src);
        IG_CHECK(dst);

        D3D12_PLACED_SUBRESOURCE_FOOTPRINT offsetedLayout = layout;
        offsetedLayout.Offset += srcOffsetInBytes;

        const CD3DX12_TEXTURE_COPY_LOCATION srcLocation(&src.GetNative(), offsetedLayout);
        const CD3DX12_TEXTURE_COPY_LOCATION dstLocation(&dst.GetNative(), subresourceIdx);
        cmdList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
    }

    void CommandList::CopyTextureSimple(GpuTexture& src, GpuTexture& dst)
    {
        IG_CHECK(src);
        IG_CHECK(dst);

        cmdList->CopyResource(&dst.GetNative(), &src.GetNative());
    }

    void CommandList::SetRootSignature(RootSignature& rootSignature)
    {
        IG_CHECK(IsValid());
        IG_CHECK(rootSignature);
        IG_CHECK(cmdListTargetQueueType == EQueueType::Graphics || cmdListTargetQueueType == EQueueType::Compute);
        switch (cmdListTargetQueueType)
        {
        case EQueueType::Graphics:
            cmdList->SetGraphicsRootSignature(&rootSignature.GetNative());
            break;

        case EQueueType::Compute:
            cmdList->SetComputeRootSignature(&rootSignature.GetNative());
            break;
        }
    }

    void CommandList::SetDescriptorHeaps(const std::span<DescriptorHeap*> descriptorHeaps)
    {
        IG_CHECK(IsValid());
        IG_CHECK(cmdListTargetQueueType == EQueueType::Graphics || cmdListTargetQueueType == EQueueType::Compute);
        auto toNative = views::all(descriptorHeaps) |
                        views::filter([](DescriptorHeap* ptr)
                                      { return ptr != nullptr; }) |
                        views::transform([](DescriptorHeap* ptr)
                                         { return &ptr->GetNative(); });
        auto nativePtrs = ToVector(toNative);
        cmdList->SetDescriptorHeaps(static_cast<U32>(nativePtrs.size()), nativePtrs.data());
    }

    void CommandList::SetDescriptorHeap(DescriptorHeap& descriptorHeap)
    {
        DescriptorHeap* descriptorHeaps[] = {&descriptorHeap};
        SetDescriptorHeaps(descriptorHeaps);
    }

    void CommandList::SetVertexBuffer(GpuBuffer& vertexBuffer)
    {
        IG_CHECK(IsValid());
        IG_CHECK(vertexBuffer);
        const std::optional<D3D12_VERTEX_BUFFER_VIEW> vbView = vertexBuffer.GetVertexBufferView();
        IG_CHECK(vbView);
        cmdList->IASetVertexBuffers(0, 1, &vbView.value());
    }

    void CommandList::SetIndexBuffer(GpuBuffer& indexBuffer)
    {
        IG_CHECK(IsValid());
        IG_CHECK(indexBuffer);
        const std::optional<D3D12_INDEX_BUFFER_VIEW> ibView = indexBuffer.GetIndexBufferView();
        IG_CHECK(ibView);
        cmdList->IASetIndexBuffer(&ibView.value());
    }

    void CommandList::SetRenderTarget(const GpuView& rtv, std::optional<std::reference_wrapper<GpuView>> dsv /*= std::nullopt*/)
    {
        IG_CHECK(IsValid());
        IG_CHECK(cmdListTargetQueueType == EQueueType::Graphics);
        IG_CHECK(rtv && (rtv.Type == EGpuViewType::RenderTargetView));
        IG_CHECK(!dsv || (dsv->get() && (dsv->get().Type == EGpuViewType::DepthStencilView)));

        const D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuHandle = rtv.CpuHandle;
        const D3D12_CPU_DESCRIPTOR_HANDLE dsvCpuHandle = dsv ? dsv->get().CpuHandle : D3D12_CPU_DESCRIPTOR_HANDLE{};

        cmdList->OMSetRenderTargets(1, &rtvCpuHandle, FALSE, dsv ? &dsvCpuHandle : nullptr);
    }

    void CommandList::SetPrimitiveTopology(const D3D12_PRIMITIVE_TOPOLOGY primitiveTopology)
    {
        IG_CHECK(IsValid());
        cmdList->IASetPrimitiveTopology(primitiveTopology);
    }

    void CommandList::SetViewport(const F32 topLeftX, const F32 topLeftY, const F32 width, const F32 height,
                                  const F32 minDepth /*= 0.f*/, const F32 maxDepth /*= 1.f*/)
    {
        IG_CHECK(IsValid());
        const D3D12_VIEWPORT viewport{topLeftX, topLeftY, width, height, minDepth, maxDepth};
        cmdList->RSSetViewports(1, &viewport);
    }

    void CommandList::SetViewport(const Viewport& viewport)
    {
        SetViewport(viewport.x, viewport.y,
                    viewport.width, viewport.height,
                    viewport.minDepth, viewport.maxDepth);
    }

    void CommandList::SetScissorRect(const long left, const long top, const long right, const long bottom)
    {
        IG_CHECK(IsValid());
        const D3D12_RECT rect{left, top, right, bottom};
        cmdList->RSSetScissorRects(1, &rect);
    }

    void CommandList::SetScissorRect(const Viewport& viewport)
    {
        SetScissorRect(
            static_cast<long>(viewport.x), static_cast<long>(viewport.y), static_cast<long>(viewport.width), static_cast<long>(viewport.height));
    }

    void CommandList::DrawIndexed(const U32 numIndices, const U32 indexOffset, const U32 vertexOffset)
    {
        IG_CHECK(IsValid());
        cmdList->DrawIndexedInstanced(numIndices, 1, indexOffset, vertexOffset, 0);
    }

    void CommandList::Dispatch(U32 threadGroupSizeX, U32 threadGroupSizeY, U32 threadGroupSizeZ)
    {
        IG_CHECK(IsValid());
        cmdList->Dispatch(threadGroupSizeX, threadGroupSizeY, threadGroupSizeZ);
    }

    void CommandList::ExecuteIndirect(CommandSignature& cmdSignature, GpuBuffer& cmdBuffer)
    {
        IG_CHECK(IsValid());
        const GpuBufferDesc& cmdBufferDesc = cmdBuffer.GetDesc();
        IG_CHECK(cmdBufferDesc.IsUavCounterEnabled());
        ID3D12CommandSignature& nativeCmdSignature = cmdSignature.GetNative();
        ID3D12Resource& nativeCmdBuffer = cmdBuffer.GetNative();
        cmdList->ExecuteIndirect(&nativeCmdSignature,
                                 cmdBufferDesc.GetNumElements(), &nativeCmdBuffer, 0,
                                 &nativeCmdBuffer, cmdBufferDesc.GetUavCounterOffset());
    }

    void CommandList::SetRoot32BitConstants(const U32 registerSlot,
                                            const U32 num32BitValuesToSet,
                                            const void* srcData,
                                            const U32 destOffsetIn32BitValues)
    {
        constexpr U32 NumMaximumRootConstants = 64;
        IG_VERIFY(num32BitValuesToSet < NumMaximumRootConstants);
        IG_CHECK((destOffsetIn32BitValues + num32BitValuesToSet) < NumMaximumRootConstants);
        IG_CHECK(srcData != nullptr);

        switch (cmdListTargetQueueType)
        {
        case EQueueType::Graphics:
            cmdList->SetGraphicsRoot32BitConstants(registerSlot, num32BitValuesToSet, srcData, destOffsetIn32BitValues);
            break;
        case EQueueType::Compute:
            cmdList->SetComputeRoot32BitConstants(registerSlot, num32BitValuesToSet, srcData, destOffsetIn32BitValues);
            break;
        default:
            IG_CHECK_NO_ENTRY();
            break;
        }
    }
} // namespace ig
