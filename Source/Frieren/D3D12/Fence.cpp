#include <D3D12/Fence.h>
#include <Core/Assert.h>

namespace fe::dx
{
	Fence::Fence(ComPtr<ID3D12Fence> newFence)
		: fence(std::move(newFence)), eventHandle(CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS))
	{
	}

	Fence::Fence(Fence&& other) noexcept
		: fence(std::move(other.fence))
		, eventHandle(std::exchange(other.eventHandle, (HANDLE)NULL))
		, counter(std::exchange(other.counter, 0))
	{
	}

	Fence::~Fence()
	{
		if (eventHandle != NULL)
		{
			CloseHandle(eventHandle);
		}
	}

	Fence& Fence::operator=(Fence&& other) noexcept
	{
		fence = std::move(other.fence);
		eventHandle = std::exchange(other.eventHandle, (HANDLE)NULL);
		counter = std::exchange(other.counter, 0);
		return *this;
	}

	void Fence::WaitOnCPU()
	{
		check(fence);
		check(eventHandle != NULL);
		const uint64_t completedValue = fence->GetCompletedValue();
		if (completedValue < counter)
		{
			verify_succeeded(fence->SetEventOnCompletion(counter, eventHandle));
			WaitForSingleObject(eventHandle, INFINITE);
		}
	}
} // namespace fe::dx