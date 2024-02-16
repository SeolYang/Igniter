#include <D3D12/CommandContext.h>
#include <D3D12/Device.h>
#include <D3D12/GPUTextureDesc.h>
#include <D3D12/GPUTexture.h>
#include <D3D12/GPUBufferDesc.h>
#include <D3D12/GPUBuffer.h>
#include <D3D12/PipelineState.h>
#include <D3D12/DescriptorHeap.h>
#include <Core/Assert.h>

namespace fe::dx
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

	CommandContext::CommandContext(ComPtr<ID3D12CommandAllocator> newCmdAllocator, ComPtr<NativeType> newCmdList,
								   const EQueueType targetQueueType)
		: cmdAllocator(std::move(newCmdAllocator))
		, cmdList(std::move(newCmdList))
		, cmdListTargetQueueType(targetQueueType)
	{
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

	void CommandContext::AddPendingTextureBarrier(GPUTexture& targetTexture, const D3D12_BARRIER_SYNC syncBefore,
												  const D3D12_BARRIER_SYNC syncAfter, D3D12_BARRIER_ACCESS accessBefore,
												  const D3D12_BARRIER_ACCESS			accessAfter,
												  const D3D12_BARRIER_LAYOUT			layoutBefore,
												  const D3D12_BARRIER_LAYOUT			layoutAfter,
												  const D3D12_BARRIER_SUBRESOURCE_RANGE subresourceRange)
	{
		check(IsValid());
		check(targetTexture);
		check(syncBefore != syncAfter || accessBefore != accessAfter || layoutBefore != layoutAfter);
		// #todo 서브리소스 범위 체크
		// const auto& desc = targetTexture.GetDesc();
		// check(...);

		// #todo 커맨드 리스트 타입별 배리어 유효성 체크
		// check(IsValidFor(cmdListTargetQueueType, accessAfter) && ...);

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

	void CommandContext::FlushPendingTextureBarriers()
	{
		check(IsValid());
		const D3D12_BARRIER_GROUP barrierGroup{ .Type = D3D12_BARRIER_TYPE_TEXTURE,
												.NumBarriers = static_cast<uint32_t>(pendingTextureBarriers.size()),
												.pTextureBarriers = pendingTextureBarriers.data() };
		cmdList->Barrier(1, &barrierGroup);
		pendingTextureBarriers.clear();
	}

	void CommandContext::AddPendingBufferBarrier(GPUBuffer& targetBuffer, const D3D12_BARRIER_SYNC syncBefore, const D3D12_BARRIER_SYNC syncAfter, D3D12_BARRIER_ACCESS accessBefore, const D3D12_BARRIER_ACCESS accessAfter, const size_t offset, const size_t sizeAsBytes)
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

	void CommandContext::FlushPendingBufferBarriers()
	{
		check(IsValid());
		const D3D12_BARRIER_GROUP barrierGroup{ .Type = D3D12_BARRIER_TYPE_BUFFER,
												.NumBarriers = static_cast<uint32_t>(pendingBufferBarriers.size()),
												.pBufferBarriers = pendingBufferBarriers.data() };
		cmdList->Barrier(1, &barrierGroup);
		pendingBufferBarriers.clear();
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

	void CommandContext::ClearRenderTarget(const Descriptor& renderTargetView, float r, float g, float b, float a)
	{
		check(IsValid());
		check(cmdListTargetQueueType == EQueueType::Direct);
		check(renderTargetView && (renderTargetView.GetType() == EDescriptorType::RenderTargetView));
		const float rgba[4] = { r, g, b, a };
		cmdList->ClearRenderTargetView(renderTargetView.GetCPUDescriptorHandle(), rgba, 0, nullptr);
	}

	void CommandContext::ClearDepthStencil(const Descriptor& depthStencilView, float depth, uint8_t stencil)
	{
		check(IsValid());
		check(cmdListTargetQueueType == EQueueType::Direct);
		check(depthStencilView && (depthStencilView.GetType() == EDescriptorType::DepthStencilView));

		const D3D12_CPU_DESCRIPTOR_HANDLE dsvCpuHandle = depthStencilView.GetCPUDescriptorHandle();
		cmdList->ClearDepthStencilView(dsvCpuHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil,
									   0, nullptr);
	}

	void CommandContext::ClearDepth(const Descriptor& depthStencilView, float depth /*= 1.f*/)
	{
		check(IsValid());
		check(cmdListTargetQueueType == EQueueType::Direct);
		check(depthStencilView && (depthStencilView.GetType() == EDescriptorType::DepthStencilView));

		const D3D12_CPU_DESCRIPTOR_HANDLE dsvCpuHandle = depthStencilView.GetCPUDescriptorHandle();
		cmdList->ClearDepthStencilView(dsvCpuHandle, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
	}

	void CommandContext::ClearStencil(const Descriptor& depthStencilView, uint8_t stencil /*= 0*/)
	{
		check(IsValid());
		check(cmdListTargetQueueType == EQueueType::Direct);
		check(depthStencilView && (depthStencilView.GetType() == EDescriptorType::DepthStencilView));

		const D3D12_CPU_DESCRIPTOR_HANDLE dsvCpuHandle = depthStencilView.GetCPUDescriptorHandle();
		cmdList->ClearDepthStencilView(dsvCpuHandle, D3D12_CLEAR_FLAG_STENCIL, 0.f, stencil, 0, nullptr);
	}

	void CommandContext::SetRenderTarget(const Descriptor&								   renderTargetView,
										 std::optional<std::reference_wrapper<Descriptor>> depthStencilView)
	{
		check(IsValid());
		check(cmdListTargetQueueType == EQueueType::Direct);
		check(renderTargetView && (renderTargetView.GetType() == EDescriptorType::RenderTargetView));
		check(!depthStencilView ||
			  (depthStencilView->get() && (depthStencilView->get().GetType() == EDescriptorType::DepthStencilView)));

		const D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuHandle = renderTargetView.GetCPUDescriptorHandle();
		const D3D12_CPU_DESCRIPTOR_HANDLE dsvCpuHandle =
			depthStencilView ? depthStencilView->get().GetCPUDescriptorHandle() : D3D12_CPU_DESCRIPTOR_HANDLE{};

		cmdList->OMSetRenderTargets(1, &rtvCpuHandle, FALSE, depthStencilView ? &dsvCpuHandle : nullptr);
	}

	void CommandContext::SetDescriptorHeap(DescriptorHeap& descriptorHeap)
	{
		check(IsValid());
		check(cmdListTargetQueueType == EQueueType::Direct || cmdListTargetQueueType == EQueueType::AsyncCompute);
		check(descriptorHeap);
		ID3D12DescriptorHeap* descriptorHeaps[] = { &descriptorHeap.GetNative() };
		cmdList->SetDescriptorHeaps(1, descriptorHeaps);
	}

	void CommandContext::CopyBuffer(GPUBuffer& from, const size_t srcOffset, const size_t numBytes, GPUBuffer& to, const size_t dstOffset)
	{
		check(IsValid());
		check(from);
		check(to);
		check(numBytes > 0);
		check(dstOffset + numBytes <= to.GetDesc().GetSizeAsBytes());
		check(srcOffset + numBytes <= from.GetDesc().GetSizeAsBytes());

		cmdList->CopyBufferRegion(&to.GetNative(), dstOffset, &from.GetNative(), srcOffset, numBytes);
	}

	void CommandContext::CopyBuffer(GPUBuffer& from, GPUBuffer& to)
	{
		const auto& srcDesc = from.GetDesc();
		CopyBuffer(from, 0, srcDesc.GetSizeAsBytes(), to, 0);
	}
} // namespace fe::dx