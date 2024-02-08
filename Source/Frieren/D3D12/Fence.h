#pragma once
#include <D3D12/Common.h>
#include <Core/Assert.h>

namespace fe::dx
{
	class Fence
	{
		friend class Device;

	public:
		Fence(const Fence&) = delete;
		Fence(Fence&& other) noexcept;
		~Fence();

		Fence& operator=(const Fence&) = delete;
		Fence& operator=(Fence&& other) noexcept;

		bool IsValid() const noexcept { return fence && eventHandle != NULL; }
		operator bool() const noexcept { return IsValid(); }

		uint64_t GetCounter() const { return counter; }

		auto& GetNative()
		{
			check(fence);
			return *fence.Get();
		}

		const auto& GetNative() const
		{
			check(fence);
			return *fence.Get();
		}

		void Next() { ++counter; }

		// #todo 각 함수를 적재적소에 배치/분배
		void Signal(ID3D12CommandQueue& cmdQueue);
		void GpuWaitForFence(ID3D12CommandQueue& cmdQueue);
		void CpuWaitForFence();

	private:
		Fence(ComPtr<ID3D12Fence> newFence);

	private:
		ComPtr<ID3D12Fence> fence;
		HANDLE				eventHandle = NULL;
		uint64_t			counter = 0;
	};
} // namespace fe::dx
