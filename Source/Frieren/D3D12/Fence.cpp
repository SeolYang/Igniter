#include <D3D12/Fence.h>
#include <Engine.h>

namespace fe::dx
{
	Fence::Fence(Device& device, const std::string_view debugName)
		: eventHandle(CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS))
	{
		auto& nativeDevice = device.GetNative();
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
		verify_succeeded(cmdQueue.Signal(fence.Get(), counter));
	}

	void Fence::GpuWaitForFence(ID3D12CommandQueue& cmdQueue)
	{
		verify_succeeded(cmdQueue.Wait(fence.Get(), counter));
	}

	void Fence::CpuWaitForFence()
	{
		const uint64_t completedValue = fence->GetCompletedValue();
		if (completedValue < counter)
		{
			verify_succeeded(fence->SetEventOnCompletion(counter, eventHandle));
			WaitForSingleObject(eventHandle, INFINITE);
		}
	}
} // namespace fe::dx