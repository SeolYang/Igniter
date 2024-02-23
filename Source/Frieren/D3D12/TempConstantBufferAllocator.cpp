#include <D3D12/TempConstantBufferAllocator.h>
#include <D3D12/Device.h>
#include <D3D12/GPUBuffer.h>
#include <D3D12/GPUBufferDesc.h>
#include <D3D12/GPUViewManager.h>
#include <Core/Assert.h>
#include <ranges>

namespace fe::dx
{
	TempConstantBufferAllocator::TempConstantBufferAllocator(const FrameManager& frameManager, Device& renderDevice, HandleManager& handleManager, GPUViewManager& gpuViewManager, const uint32_t reservedSizeInBytesPerFrame)
		: frameManager(frameManager),
		  renderDevice(renderDevice),
		  handleManager(handleManager),
		  gpuViewManager(gpuViewManager),
		  reservedSizeInBytesPerFrame(reservedSizeInBytesPerFrame)
	{
		check(reservedSizeInBytesPerFrame % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0);

		GPUBufferDesc desc{};
		desc.AsConstantBuffer(reservedSizeInBytesPerFrame);

		for (const uint8_t localFrameIdx : std::views::iota(0Ui8, NumFramesInFlight))
		{
			allocatedSizeInBytes[localFrameIdx].store(0);
			buffers.emplace_back(renderDevice.CreateBuffer(desc).value());
		}

		// #wip_todo State transition 은 어떻게 처리하지?
	}

	TempConstantBufferAllocator::~TempConstantBufferAllocator()
	{
	}

	TempConstantBuffer TempConstantBufferAllocator::Allocate(const GPUBufferDesc& desc)
	{
		const size_t currentLocalFrameIdx = frameManager.GetLocalFrameIndex();
		check(currentLocalFrameIdx < NumFramesInFlight);
		const uint64_t allocSizeInBytes = desc.GetSizeAsBytes();
		const uint64_t offset = allocatedSizeInBytes[currentLocalFrameIdx].fetch_add(allocSizeInBytes);
		check(offset <= reservedSizeInBytesPerFrame);
		mappedBuffers[currentLocalFrameIdx].emplace_back(buffers[currentLocalFrameIdx].MapHandle(handleManager, 0, { offset, allocSizeInBytes }));
		allocatedViews[currentLocalFrameIdx].emplace_back(gpuViewManager.RequestConstantBufferView(buffers[currentLocalFrameIdx], offset, allocSizeInBytes));

		return TempConstantBuffer{
			.Mapping = mappedBuffers[currentLocalFrameIdx].back().DeriveWeak(),
			.View = allocatedViews[currentLocalFrameIdx].back().DeriveWeak()
		};
	}

	void TempConstantBufferAllocator::DeallocateCurrentFrame()
	{
		const size_t currentLocalFrameIdx = frameManager.GetLocalFrameIndex();
		check(currentLocalFrameIdx < NumFramesInFlight);
		mappedBuffers[currentLocalFrameIdx].clear();
		allocatedViews[currentLocalFrameIdx].clear();
		allocatedSizeInBytes[currentLocalFrameIdx].store(0);
	}
} // namespace fe::dx