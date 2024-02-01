#pragma once
#include <D3D12/Device.h>

namespace fe::dx
{
	class Fence
	{
	public:
		Fence(Device& device, std::string_view debugName);
		~Fence();

		Fence(const Fence&) = delete;
		Fence(Fence&&) noexcept = delete;

		Fence& operator=(const Fence&) = delete;
		Fence& operator=(Fence&&) noexcept = delete;

		uint64_t			GetCounter() const { return counter; }
		[[nodiscard]] auto& GetNative() { return *fence.Get(); }

		void Next() { ++counter; }
		// #todo Moved to device
		void Signal(ID3D12CommandQueue& cmdQueue);
		void GpuWaitForFence(ID3D12CommandQueue& cmdQueue);
		void CpuWaitForFence();

	private:
		HANDLE				eventHandle;
		uint64_t			counter = 0;
		ComPtr<ID3D12Fence> fence;
	};
} // namespace fe::dx
