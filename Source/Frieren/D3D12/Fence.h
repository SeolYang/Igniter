#pragma once
#include <D3D12/Device.h>

namespace fe
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

		void Next() { ++counter; }
		void Signal(ID3D12CommandQueue& cmdQueue);
		void GpuWaitForFence(ID3D12CommandQueue& cmdQueue);
		void CpuWaitForFence();

	private:
		HANDLE					 eventHandle;
		uint64_t				 counter = 0;
		wrl::ComPtr<ID3D12Fence> fence;
	};
} // namespace fe
