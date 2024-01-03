#include <D3D12/Fence.h>
#include <Engine.h>

namespace fe
{
	Fence::Fence(Device& device, const std::string_view debugName)
		: eventHandle(CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS))
	{
		ID3D12Device10& nativeDevice = device.GetNative();
		const bool	   bIsFenceCreated = IsDXCallSucceeded(nativeDevice.CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
		FE_CONDITIONAL_LOG(D3D12Fatal, bIsFenceCreated, "Failed to create fence.");

		Private::SetD3DObjectName(fence.Get(), debugName);
	}

	Fence::~Fence()
	{
		CloseHandle(eventHandle);
	}

	void Fence::Signal(ID3D12CommandQueue& cmdQueue)
	{
		const bool bSignalSucceeded = IsDXCallSucceeded(cmdQueue.Signal(fence.Get(), counter));
		FE_LOG(D3D12Warn, "Failed to signal fence to {}", counter);
	}

	void Fence::GpuWaitForFence(ID3D12CommandQueue& cmdQueue)
	{
		const bool bIsValidWait = IsDXCallSucceeded(cmdQueue.Wait(fence.Get(), counter));
		FE_LOG(D3D12Warn, "This is not valid wait request.");
	}

	void Fence::CpuWaitForFence()
	{
		const uint64_t completedValue = fence->GetCompletedValue();
		if (completedValue < counter)
		{
			FE_CONDITIONAL_LOG(D3D12Fatal, eventHandle != NULL, "Invalid Event Handle.");
			const bool bSetEventSucceded = IsDXCallSucceeded(fence->SetEventOnCompletion(counter, eventHandle));
			FE_CONDITIONAL_LOG(D3D12Fatal, bSetEventSucceded, "Failed to set event on completion of fence.");
			if (bSetEventSucceded)
			{
				::WaitForSingleObject(eventHandle, INFINITE);
			}
		}
	}
} // namespace fe