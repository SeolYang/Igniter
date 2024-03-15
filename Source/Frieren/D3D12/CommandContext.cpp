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
#include <Core/Assert.h>

namespace fe
{
    CommandContext::CommandContext(CommandContext&& other) noexcept
        : cmdAllocator(std::move(other.cmdAllocator)),
          cmdList(std::move(other.cmdList)),
          cmdListTargetQueueType(other.cmdListTargetQueueType),
          pendingGlobalBarriers(std::move(other.pendingGlobalBarriers)),
          pendingTextureBarriers(std::move(other.pendingTextureBarriers)),
          pendingBufferBarriers(std::move(other.pendingBufferBarriers))
    {
    }

    CommandContext::CommandContext(ComPtr<ID3D12CommandAllocator> newCmdAllocator, ComPtr<NativeType> newCmdList,
                                   const EQueueType targetQueueType)
        : cmdAllocator(std::move(newCmdAllocator)),
          cmdList(std::move(newCmdList)),
          cmdListTargetQueueType(targetQueueType)
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
        verify(cmdAllocator.Get() != nullptr);
        verify(cmdList.Get() != nullptr);

        auto* const nativeInitStatePtr = initStatePtr != nullptr ? &initStatePtr->GetNative() : nullptr;
        verify(initStatePtr == nullptr || (initStatePtr != nullptr && nativeInitStatePtr != nullptr));
        verify_succeeded(cmdAllocator->Reset());
        verify_succeeded(cmdList->Reset(cmdAllocator.Get(), nativeInitStatePtr));
    }

    void CommandContext::End()
    {
        verify(cmdList.Get() != nullptr);
        verify_succeeded(cmdList->Close());
    }

    void CommandContext::AddPendingTextureBarrier(GpuTexture& targetTexture, const D3D12_BARRIER_SYNC syncBefore,
                                                  const D3D12_BARRIER_SYNC syncAfter, D3D12_BARRIER_ACCESS accessBefore,
                                                  const D3D12_BARRIER_ACCESS accessAfter,
                                                  const D3D12_BARRIER_LAYOUT layoutBefore,
                                                  const D3D12_BARRIER_LAYOUT layoutAfter,
                                                  const D3D12_BARRIER_SUBRESOURCE_RANGE subresourceRange)
    {
        check(IsValid());
        check(targetTexture);
        check(syncBefore != syncAfter || accessBefore != accessAfter || layoutBefore != layoutAfter);

        pendingTextureBarriers.emplace_back(D3D12_TEXTURE_BARRIER{ .SyncBefore = syncBefore,
                                                                   .SyncAfter = syncAfter,
                                                                   .AccessBefore = accessBefore,
                                                                   .AccessAfter = accessAfter,
                                                                   .LayoutBefore = layoutBefore,
                                                                   .LayoutAfter = layoutAfter,
                                                                   .pResource = &targetTexture.GetNative(),
                                                                   .Subresources = subresourceRange,
                                                                   .Flags = D3D12_TEXTURE_BARRIER_FLAG_NONE });
    }

    void CommandContext::AddPendingBufferBarrier(GpuBuffer& targetBuffer, const D3D12_BARRIER_SYNC syncBefore, const D3D12_BARRIER_SYNC syncAfter, D3D12_BARRIER_ACCESS accessBefore, const D3D12_BARRIER_ACCESS accessAfter, const size_t offset, const size_t sizeAsBytes)
    {
        check(IsValid());
        check(targetBuffer);
        check(syncBefore != syncAfter || accessBefore != accessAfter);

        pendingBufferBarriers.emplace_back(D3D12_BUFFER_BARRIER{ .SyncBefore = syncBefore,
                                                                 .SyncAfter = syncAfter,
                                                                 .AccessBefore = accessBefore,
                                                                 .AccessAfter = accessAfter,
                                                                 .pResource = &targetBuffer.GetNative(),
                                                                 .Offset = offset,
                                                                 .Size = sizeAsBytes });
    }

    void CommandContext::FlushBarriers()
    {
        check(IsValid());
        const D3D12_BARRIER_GROUP barrierGroups[3] = {
            { .Type = D3D12_BARRIER_TYPE_GLOBAL, .NumBarriers = static_cast<uint32_t>(pendingGlobalBarriers.size()), .pGlobalBarriers = pendingGlobalBarriers.data() },
            { .Type = D3D12_BARRIER_TYPE_TEXTURE, .NumBarriers = static_cast<uint32_t>(pendingTextureBarriers.size()), .pTextureBarriers = pendingTextureBarriers.data() },
            { .Type = D3D12_BARRIER_TYPE_BUFFER, .NumBarriers = static_cast<uint32_t>(pendingBufferBarriers.size()), .pBufferBarriers = pendingBufferBarriers.data() }
        };

        cmdList->Barrier(3, barrierGroups);
        pendingGlobalBarriers.clear();
        pendingTextureBarriers.clear();
        pendingBufferBarriers.clear();
    }

    void CommandContext::ClearRenderTarget(const GpuView& rtv, float r /*= 0.f*/, float g /*= 0.f*/, float b /*= 0.f*/, float a /*= 1.f*/)
    {
        check(IsValid());
        check(cmdListTargetQueueType == EQueueType::Direct);
        check(rtv && (rtv.Type == EGpuViewType::RenderTargetView));
        const float rgba[4] = { r, g, b, a };
        cmdList->ClearRenderTargetView(rtv.CPUHandle, rgba, 0, nullptr);
    }

    void CommandContext::ClearDepthStencil(const GpuView& dsv, float depth /*= 1.f*/, uint8_t stencil /*= 0*/)
    {
        check(IsValid());
        check(cmdListTargetQueueType == EQueueType::Direct);
        check(dsv && (dsv.Type == EGpuViewType::DepthStencilView));

        const D3D12_CPU_DESCRIPTOR_HANDLE dsvCpuHandle = dsv.CPUHandle;
        cmdList->ClearDepthStencilView(dsvCpuHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil,
                                       0, nullptr);
    }

    void CommandContext::ClearDepth(const GpuView& dsv, float depth /*= 1.f*/)
    {
        check(IsValid());
        check(cmdListTargetQueueType == EQueueType::Direct);
        check(dsv && (dsv.Type == EGpuViewType::DepthStencilView));

        const D3D12_CPU_DESCRIPTOR_HANDLE dsvCpuHandle = dsv.CPUHandle;
        cmdList->ClearDepthStencilView(dsvCpuHandle, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
    }

    void CommandContext::ClearStencil(const GpuView& dsv, uint8_t stencil /*= 0*/)
    {
        check(IsValid());
        check(cmdListTargetQueueType == EQueueType::Direct);
        check(dsv && (dsv.Type == EGpuViewType::DepthStencilView));

        const D3D12_CPU_DESCRIPTOR_HANDLE dsvCpuHandle = dsv.CPUHandle;
        cmdList->ClearDepthStencilView(dsvCpuHandle, D3D12_CLEAR_FLAG_STENCIL, 0.f, stencil, 0, nullptr);
    }

    void CommandContext::CopyBuffer(GpuBuffer& src, const size_t srcOffsetInBytes, const size_t numBytes, GpuBuffer& dst, const size_t dstOffsetInBytes)
    {
        check(IsValid());
        check(src);
        check(dst);
        check(numBytes > 0);
        check(dstOffsetInBytes + numBytes <= dst.GetDesc().GetSizeAsBytes());
        check(srcOffsetInBytes + numBytes <= src.GetDesc().GetSizeAsBytes());

        cmdList->CopyBufferRegion(&dst.GetNative(), dstOffsetInBytes, &src.GetNative(), srcOffsetInBytes, numBytes);
    }

    void CommandContext::CopyBuffer(GpuBuffer& src, GpuBuffer& dst)
    {
        const auto& srcDesc = src.GetDesc();
        CopyBuffer(src, 0, srcDesc.GetSizeAsBytes(), dst, 0);
    }

    void CommandContext::CopyTextureRegion(GpuBuffer& src, const size_t srcOffsetInBytes, GpuTexture& dst, const uint32_t subresourceIdx, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& layout)
    {
        check(src);
        check(dst);

        D3D12_PLACED_SUBRESOURCE_FOOTPRINT offsetedLayout = layout;
        offsetedLayout.Offset += srcOffsetInBytes;

        const CD3DX12_TEXTURE_COPY_LOCATION srcLocation(&src.GetNative(), offsetedLayout);
        const CD3DX12_TEXTURE_COPY_LOCATION dstLocation(&dst.GetNative(), subresourceIdx);
        cmdList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
    }

    void CommandContext::SetRootSignature(RootSignature& rootSignature)
    {
        check(IsValid());
        check(rootSignature);
        check(cmdListTargetQueueType == EQueueType::Direct || cmdListTargetQueueType == EQueueType::AsyncCompute);
        switch (cmdListTargetQueueType)
        {
            case EQueueType::Direct:
                cmdList->SetGraphicsRootSignature(&rootSignature.GetNative());
                break;

            case EQueueType::AsyncCompute:
                cmdList->SetComputeRootSignature(&rootSignature.GetNative());
                break;
        }
    }

    void CommandContext::SetDescriptorHeaps(const std::span<std::reference_wrapper<DescriptorHeap>> targetDescriptorHeaps)
    {
        check(IsValid());
        check(cmdListTargetQueueType == EQueueType::Direct || cmdListTargetQueueType == EQueueType::AsyncCompute);
        std::vector<ID3D12DescriptorHeap*> descriptorHeaps(targetDescriptorHeaps.size());
        std::transform(targetDescriptorHeaps.begin(), targetDescriptorHeaps.end(), descriptorHeaps.begin(),
                       [](DescriptorHeap& descriptorHeap)
                       { check(descriptorHeap); return &descriptorHeap.GetNative(); });
        cmdList->SetDescriptorHeaps(static_cast<uint32_t>(descriptorHeaps.size()), descriptorHeaps.data());
    }

    void CommandContext::SetDescriptorHeap(DescriptorHeap& descriptorHeap)
    {
        std::reference_wrapper<DescriptorHeap> descriptorHeaps[] = { std::ref(descriptorHeap) };
        SetDescriptorHeaps(descriptorHeaps);
    }

    void CommandContext::SetVertexBuffer(GpuBuffer& vertexBuffer)
    {
        check(IsValid());
        check(vertexBuffer);
        const std::optional<D3D12_VERTEX_BUFFER_VIEW> vbView = vertexBuffer.GetVertexBufferView();
        check(vbView);
        cmdList->IASetVertexBuffers(0, 1, &vbView.value());
    }

    void CommandContext::SetIndexBuffer(GpuBuffer& indexBuffer)
    {
        check(IsValid());
        check(indexBuffer);
        const std::optional<D3D12_INDEX_BUFFER_VIEW> ibView = indexBuffer.GetIndexBufferView();
        check(ibView);
        cmdList->IASetIndexBuffer(&ibView.value());
    }

    void CommandContext::SetRenderTarget(const GpuView& rtv, std::optional<std::reference_wrapper<GpuView>> dsv /*= std::nullopt*/)
    {
        check(IsValid());
        check(cmdListTargetQueueType == EQueueType::Direct);
        check(rtv && (rtv.Type == EGpuViewType::RenderTargetView));
        check(!dsv ||
              (dsv->get() && (dsv->get().Type == EGpuViewType::DepthStencilView)));

        const D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuHandle = rtv.CPUHandle;
        const D3D12_CPU_DESCRIPTOR_HANDLE dsvCpuHandle =
            dsv ? dsv->get().CPUHandle : D3D12_CPU_DESCRIPTOR_HANDLE{};

        cmdList->OMSetRenderTargets(1, &rtvCpuHandle, FALSE, dsv ? &dsvCpuHandle : nullptr);
    }

    void CommandContext::SetPrimitiveTopology(const D3D12_PRIMITIVE_TOPOLOGY primitiveTopology)
    {
        check(IsValid());
        cmdList->IASetPrimitiveTopology(primitiveTopology);
    }

    void CommandContext::SetViewport(const float topLeftX, const float topLeftY, const float width, const float height, const float minDepth /*= 0.f*/, const float maxDepth /*= 1.f*/)
    {
        check(IsValid());
        const D3D12_VIEWPORT viewport{ topLeftX, topLeftY, width, height, minDepth, maxDepth };
        cmdList->RSSetViewports(1, &viewport);
    }

    void CommandContext::SetViewport(const Viewport& viewport)
    {
        SetViewport(viewport.x, viewport.y, viewport.width, viewport.height, viewport.minDepth, viewport.maxDepth);
    }

    void CommandContext::SetScissorRect(const long left, const long top, const long right, const long bottom)
    {
        check(IsValid());
        const D3D12_RECT rect{ left, top, right, bottom };
        cmdList->RSSetScissorRects(1, &rect);
    }

    void CommandContext::SetScissorRect(const Viewport& viewport)
    {
        SetScissorRect(static_cast<long>(viewport.x), static_cast<long>(viewport.y), static_cast<long>(viewport.width), static_cast<long>(viewport.height));
    }

    void CommandContext::DrawIndexed(const uint32_t numIndices, const uint32_t indexOffset, const uint32_t vertexOffset)
    {
        check(IsValid());
        cmdList->DrawIndexedInstanced(numIndices, 1, indexOffset, vertexOffset, 0);
    }

    void CommandContext::SetRoot32BitConstants(const uint32_t registerSlot, const uint32_t num32BitValuesToSet, const void* srcData, const uint32_t destOffsetIn32BitValues)
    {
        constexpr uint32_t NumMaximumRootConstants = 64;
        verify(num32BitValuesToSet < NumMaximumRootConstants);
        check((destOffsetIn32BitValues + num32BitValuesToSet) < NumMaximumRootConstants);

        if (srcData != nullptr)
        {
            switch (cmdListTargetQueueType)
            {
                case EQueueType::Direct:
                    cmdList->SetGraphicsRoot32BitConstants(registerSlot, num32BitValuesToSet, srcData, destOffsetIn32BitValues);
                    break;
                case EQueueType::AsyncCompute:
                    cmdList->SetComputeRoot32BitConstants(registerSlot, num32BitValuesToSet, srcData, destOffsetIn32BitValues);
                    break;
                default:
                    checkNoEntry();
                    break;
            }
        }
        else
        {
            checkNoEntry();
        }
    }

} // namespace fe