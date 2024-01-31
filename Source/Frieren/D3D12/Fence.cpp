#include <D3D12/Fence.h>
#include <Engine.h>

namespace fe::dx
{
	Fence::Fence(const Device& device, const std::string_view debugName)
		: eventHandle(CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS))
	{
		ID3D12Device10& nativeDevice = device.GetNative();
		const bool		bIsFenceCreated = SUCCEEDED(nativeDevice.CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
		FE_CONDITIONAL_LOG(D3D12Fatal, bIsFenceCreated, "Failed to create fence.");

		SetObjectName(fence.Get(), debugName);
	}

	Fence::~Fence()
	{
		CloseHandle(eventHandle);
	}

	void Fence::Signal(ID3D12CommandQueue& cmdQueue)
	{
		const bool bSignalSucceeded = SUCCEEDED(cmdQueue.Signal(fence.Get(), counter));
		FE_LOG(D3D12Warn, "Failed to signal fence to {}", counter);
	}

	void Fence::GpuWaitForFence(ID3D12CommandQueue& cmdQueue)
	{
		const bool bIsValidWait = SUCCEEDED(cmdQueue.Wait(fence.Get(), counter));
		FE_LOG(D3D12Warn, "This is not valid wait request.");
	}

	void Fence::CpuWaitForFence()
	{
		const uint64_t completedValue = fence->GetCompletedValue();
		if (completedValue < counter)
		{
			check(SUCCEEDED(fence->SetEventOnCompletion(counter, eventHandle)));
			WaitForSingleObject(eventHandle, INFINITE);
		}
	}
} // namespace fe::dx