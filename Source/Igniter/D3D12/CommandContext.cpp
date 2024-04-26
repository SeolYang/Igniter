#include <Igniter.h>
#include <Core/ContainerUtils.h>
#include <D3D12/CommandContext.h>
#include <D3D12/RenderDevice.h>
#include <D3D12/GPUTextureDesc.h>
#include <D3D12/GPUTexture.h>
#include <D3D12/GPUBufferDesc.h>
#include <D3D12/GPUBuffer.h>
#include <D3D12/GPUView.h>
#include <D3D12/PipelineState.h>
#include <D3D12/DescriptorHeap.h>
#include <D3D12/RootSignature.h>

namespace ig
{
    CommandContext::CommandContext(CommandContext&& other) noexcept
        : cmdAllocator(std::move(other.cmdAllocator))
        , cmdList(std::move(other.cmdList))
        , cmdListTargetQueueType(other.cmdListTargetQueueType)
        , pendingGlobalBarriers(std::move(other.pendingGlobalBarriers))
        , pendingTextureBarriers(std::move(other.pendingTextureBarriers))
        , pendingBufferBarriers(std::move(other.pendingBufferBarriers))
    {
    }

    CommandContext::CommandContext(ComPtr<ID3D12CommandAllocator> newCmdAllocator, ComPtr<NativeType> newCmdList, const EQueueType targetQueueType)
        : cmdAllocator(std::move(newCmdAllocator)), cmdList(std::move(newCmdList)), cmdListTargetQueueType(targetQueueType)
    {
    }

    CommandContext& CommandContext::operator=(CommandContext&& other) noexcept
    {
        cmdAllocator = std::move(other.cmdAllocator);
        cmdList = std::move(other.cmdList);
        cmdListTargetQueueType = other.cmdListTargetQueueType;
        pendingGlobalBarriers = std::move(other.pendingGlobalBarriers);
        pendingTextureBarriers = std::move(other.pendingTextureBarriers);
        pendingBufferBarriers = std::move(other.pendingBufferBarriers);
        return *this;
    }

    void CommandContext::Begin(PipelineState* const initStatePtr)
    {
        IG_VERIFY(cmdAllocator.Get() != nullptr);
        IG_VERIFY(cmdList.Get() != nullptr);

        auto* const nativeInitStatePtr = initStatePtr != nullptr ? &initStatePtr->GetNative() : nullptr;
        IG_VERIFY(initStatePtr == nullptr || (initStatePtr != nullptr && nativeInitStatePtr != nullptr));
        IG_VERIFY_SUCCEEDED(cmdAllocator->Reset());
        IG_VERIFY_SUCCEEDED(cmdList->Reset(cmdAllocator.Get(), nativeInitStatePtr));
    }

    void CommandContext::End()
    {
        IG_VERIFY(cmdList.Get() != nullptr);
        IG_VERIFY_SUCCEEDED(cmdList->Close());
    }

    void CommandContext::AddPendingTextureBarrier(GpuTexture& targetTexture, const D3D12_BARRIER_SYNC syncBefore, const D3D12_BARRIER_SYNC syncAfter,
        D3D12_BARRIER_ACCESS accessBefore, const D3D12_BARRIER_ACCESS accessAfter, const D3D12_BARRIER_LAYOUT layoutBefore,
        const D3D12_BARRIER_LAYOUT layoutAfter, const D3D12_BARRIER_SUBRESOURCE_RANGE subresourceRange)
    {
        IG_CHECK(IsValid());
        IG_CHECK(targetTexture);
        IG_CHECK(syncBefore != syncAfter || accessBefore != accessAfter || layoutBefore != layoutAfter);

        pendingTextureBarriers.emplace_back(D3D12_TEXTURE_BARRIER{.SyncBefore = syncBefore,
            .SyncAfter = syncAfter,
            .AccessBefore = accessBefore,
            .AccessAfter = accessAfter,
            .LayoutBefore = layoutBefore,
            .LayoutAfter = layoutAfter,
            .pResource = &targetTexture.GetNative(),
            .Subresources = subresourceRange,
            .Flags = D3D12_TEXTURE_BARRIER_FLAG_NONE});
    }

    void CommandContext::AddPendingBufferBarrier(GpuBuffer& targetBuffer, const D3D12_BARRIER_SYNC syncBefore, const D3D12_BARRIER_SYNC syncAfter,
        D3D12_BARRIER_ACCESS accessBefore, const D3D12_BARRIER_ACCESS accessAfter, const size_t offset, const size_t sizeAsBytes)
    {
        IG_CHECK(IsValid());
        IG_CHECK(targetBuffer);
        IG_CHECK(syncBefore != syncAfter || accessBefore != accessAfter);

        pendingBufferBarriers.emplace_back(D3D12_BUFFER_BARRIER{.SyncBefore = syncBefore,
            .SyncAfter = syncAfter,
            .AccessBefore = accessBefore,
            .AccessAfter = accessAfter,
            .pResource = &targetBuffer.GetNative(),
            .Offset = offset,
            .Size = sizeAsBytes});
    }

    void CommandContext::FlushBarriers()
    {
        IG_CHECK(IsValid());
        const D3D12_BARRIER_GROUP barrierGroups[3] = {{.Type = D3D12_BARRIER_TYPE_GLOBAL,
                                                          .NumBarriers = static_cast<uint32_t>(pendingGlobalBarriers.size()),
                                                          .pGlobalBarriers = pendingGlobalBarriers.data()},
            {.Type = D3D12_BARRIER_TYPE_TEXTURE,
                .NumBarriers = static_cast<uint32_t>(pendingTextureBarriers.size()),
                .pTextureBarriers = pendingTextureBarriers.data()},
            {.Type = D3D12_BARRIER_TYPE_BUFFER,
                .NumBarriers = static_cast<uint32_t>(pendingBufferBarriers.size()),
                .pBufferBarriers = pendingBufferBarriers.data()}};

        cmdList->Barrier(3, barrierGroups);
        pendingGlobalBarriers.clear();
        pendingTextureBarriers.clear();
        pendingBufferBarriers.clear();
    }

    void CommandContext::ClearRenderTarget(const GpuView& rtv, float r /*= 0.f*/, float g /*= 0.f*/, float b /*= 0.f*/, float a /*= 1.f*/)
    {
        IG_CHECK(IsValid());
        IG_CHECK(cmdListTargetQueueType == EQueueType::Graphics);
        IG_CHECK(rtv && (rtv.Type == EGpuViewType::RenderTargetView));
        const float rgba[4] = {r, g, b, a};
        cmdList->ClearRenderTargetView(rtv.CPUHandle, rgba, 0, nullptr);
    }

    void CommandContext::ClearDepthStencil(const GpuView& dsv, float depth /*= 1.f*/, uint8_t stencil /*= 0*/)
    {
        IG_CHECK(IsValid());
        IG_CHECK(cmdListTargetQueueType == EQueueType::Graphics);
        IG_CHECK(dsv && (dsv.Type == EGpuViewType::DepthStencilView));

        const D3D12_CPU_DESCRIPTOR_HANDLE dsvCpuHandle = dsv.CPUHandle;
        cmdList->ClearDepthStencilView(dsvCpuHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, 0, nullptr);
    }

    void CommandContext::ClearDepth(const GpuView& dsv, float depth /*= 1.f*/)
    {
        IG_CHECK(IsValid());
        IG_CHECK(cmdListTargetQueueType == EQueueType::Graphics);
        IG_CHECK(dsv && (dsv.Type == EGpuViewType::DepthStencilView));

        const D3D12_CPU_DESCRIPTOR_HANDLE dsvCpuHandle = dsv.CPUHandle;
        cmdList->ClearDepthStencilView(dsvCpuHandle, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
    }

    void CommandContext::ClearStencil(const GpuView& dsv, uint8_t stencil /*= 0*/)
    {
        IG_CHECK(IsValid());
        IG_CHECK(cmdListTargetQueueType == EQueueType::Graphics);
        IG_CHECK(dsv && (dsv.Type == EGpuViewType::DepthStencilView));

        const D3D12_CPU_DESCRIPTOR_HANDLE dsvCpuHandle = dsv.CPUHandle;
        cmdList->ClearDepthStencilView(dsvCpuHandle, D3D12_CLEAR_FLAG_STENCIL, 0.f, stencil, 0, nullptr);
    }

    void CommandContext::CopyBuffer(
        GpuBuffer& src, const size_t srcOffsetInBytes, const size_t numBytes, GpuBuffer& dst, const size_t dstOffsetInBytes)
    {
        IG_CHECK(IsValid());
        IG_CHECK(src);
        IG_CHECK(dst);
        IG_CHECK(numBytes > 0);
        IG_CHECK(dstOffsetInBytes + numBytes <= dst.GetDesc().GetSizeAsBytes());
        IG_CHECK(srcOffsetInBytes + numBytes <= src.GetDesc().GetSizeAsBytes());

        cmdList->CopyBufferRegion(&dst.GetNative(), dstOffsetInBytes, &src.GetNative(), srcOffsetInBytes, numBytes);
    }

    void CommandContext::CopyBuffer(GpuBuffer& src, GpuBuffer& dst)
    {
        const auto& srcDesc = src.GetDesc();
        CopyBuffer(src, 0, srcDesc.GetSizeAsBytes(), dst, 0);
    }

    void CommandContext::CopyTextureRegion(GpuBuffer& src, const size_t srcOffsetInBytes, GpuTexture& dst, const uint32_t subresourceIdx,
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

    void CommandContext::SetRootSignature(RootSignature& rootSignature)
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

    void CommandContext::SetDescriptorHeaps(const std::span<DescriptorHeap*> descriptorHeaps)
    {
        IG_CHECK(IsValid());
        IG_CHECK(cmdListTargetQueueType == EQueueType::Graphics || cmdListTargetQueueType == EQueueType::Compute);
        auto toNative = views::all(descriptorHeaps) | views::filter([](DescriptorHeap* ptr) { return ptr != nullptr; }) |
                        views::transform([](DescriptorHeap* ptr) { return &ptr->GetNative(); });
        auto nativePtrs = ToVector(toNative);
        cmdList->SetDescriptorHeaps(static_cast<uint32_t>(nativePtrs.size()), nativePtrs.data());
    }

    void CommandContext::SetDescriptorHeap(DescriptorHeap& descriptorHeap)
    {
        DescriptorHeap* descriptorHeaps[] = {&descriptorHeap};
        SetDescriptorHeaps(descriptorHeaps);
    }

    void CommandContext::SetVertexBuffer(GpuBuffer& vertexBuffer)
    {
        IG_CHECK(IsValid());
        IG_CHECK(vertexBuffer);
        const std::optional<D3D12_VERTEX_BUFFER_VIEW> vbView = vertexBuffer.GetVertexBufferView();
        IG_CHECK(vbView);
        cmdList->IASetVertexBuffers(0, 1, &vbView.value());
    }

    void CommandContext::SetIndexBuffer(GpuBuffer& indexBuffer)
    {
        IG_CHECK(IsValid());
        IG_CHECK(indexBuffer);
        const std::optional<D3D12_INDEX_BUFFER_VIEW> ibView = indexBuffer.GetIndexBufferView();
        IG_CHECK(ibView);
        cmdList->IASetIndexBuffer(&ibView.value());
    }

    void CommandContext::SetRenderTarget(const GpuView& rtv, std::optional<std::reference_wrapper<GpuView>> dsv /*= std::nullopt*/)
    {
        IG_CHECK(IsValid());
        IG_CHECK(cmdListTargetQueueType == EQueueType::Graphics);
        IG_CHECK(rtv && (rtv.Type == EGpuViewType::RenderTargetView));
        IG_CHECK(!dsv || (dsv->get() && (dsv->get().Type == EGpuViewType::DepthStencilView)));

        const D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuHandle = rtv.CPUHandle;
        const D3D12_CPU_DESCRIPTOR_HANDLE dsvCpuHandle = dsv ? dsv->get().CPUHandle : D3D12_CPU_DESCRIPTOR_HANDLE{};

        cmdList->OMSetRenderTargets(1, &rtvCpuHandle, FALSE, dsv ? &dsvCpuHandle : nullptr);
    }

    void CommandContext::SetPrimitiveTopology(const D3D12_PRIMITIVE_TOPOLOGY primitiveTopology)
    {
        IG_CHECK(IsValid());
        cmdList->IASetPrimitiveTopology(primitiveTopology);
    }

    void CommandContext::SetViewport(const float topLeftX, const float topLeftY, const float width, const float height,
        const float minDepth /*= 0.f*/, const float maxDepth /*= 1.f*/)
    {
        IG_CHECK(IsValid());
        const D3D12_VIEWPORT viewport{topLeftX, topLeftY, width, height, minDepth, maxDepth};
        cmdList->RSSetViewports(1, &viewport);
    }

    void CommandContext::SetViewport(const Viewport& viewport)
    {
        SetViewport(viewport.x, viewport.y, viewport.width, viewport.height, viewport.minDepth, viewport.maxDepth);
    }

    void CommandContext::SetScissorRect(const long left, const long top, const long right, const long bottom)
    {
        IG_CHECK(IsValid());
        const D3D12_RECT rect{left, top, right, bottom};
        cmdList->RSSetScissorRects(1, &rect);
    }

    void CommandContext::SetScissorRect(const Viewport& viewport)
    {
        SetScissorRect(
            static_cast<long>(viewport.x), static_cast<long>(viewport.y), static_cast<long>(viewport.width), static_cast<long>(viewport.height));
    }

    void CommandContext::DrawIndexed(const uint32_t numIndices, const uint32_t indexOffset, const uint32_t vertexOffset)
    {
        IG_CHECK(IsValid());
        cmdList->DrawIndexedInstanced(numIndices, 1, indexOffset, vertexOffset, 0);
    }

    void CommandContext::SetRoot32BitConstants(
        const uint32_t registerSlot, const uint32_t num32BitValuesToSet, const void* srcData, const uint32_t destOffsetIn32BitValues)
    {
        constexpr uint32_t NumMaximumRootConstants = 64;
        IG_VERIFY(num32BitValuesToSet < NumMaximumRootConstants);
        IG_CHECK((destOffsetIn32BitValues + num32BitValuesToSet) < NumMaximumRootConstants);

        if (srcData != nullptr)
        {
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
        else
        {
            IG_CHECK_NO_ENTRY();
        }
    }
}    // namespace ig
