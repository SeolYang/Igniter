#pragma once
#include <D3D12/Common.h>
#include <Core/Container.h>
#include <Core/Mutex.h>

namespace fe
{
	class GpuSync final
	{
		friend class CommandQueue;

	public:
		GpuSync() = default;
		GpuSync(const GpuSync&) = default;
		GpuSync(GpuSync&&) noexcept = default;
		~GpuSync() = default;

		GpuSync& operator=(const GpuSync& rhs) = default;
		GpuSync& operator=(GpuSync&&) noexcept = default;

		[[nodiscard]] bool IsValid() const { return fence != nullptr && syncPoint > 0; }
		[[nodiscard]] operator bool() const { return IsValid(); }

		[[nodiscard]] size_t GetSyncPoint() const { return syncPoint; }

		void WaitOnCpu()
		{
			check(IsValid());
			if (fence != nullptr && syncPoint > fence->GetCompletedValue())
			{
				fence->SetEventOnCompletion(syncPoint, nullptr);
			}
		}

	private:
		GpuSync(ID3D12Fence& fence, const size_t syncPoint)
			: fence(&fence),
			  syncPoint(syncPoint)
		{
			check(IsValid());
		}

		[[nodiscard]] ID3D12Fence& GetFence()
		{
			check(fence != nullptr);
			return *fence;
		}

	private:
		ID3D12Fence* fence = nullptr;
		size_t syncPoint = 0;
	};

	class RenderDevice;
	class Fence;
	class CommandContext;
	class CommandQueue final
	{
		friend class RenderDevice;

	public:
		CommandQueue(const CommandQueue&) = delete;
		CommandQueue(CommandQueue&& other) noexcept;
		~CommandQueue();

		CommandQueue& operator=(const CommandQueue&) = delete;
		CommandQueue& operator=(CommandQueue&&) noexcept = delete;

		bool IsValid() const { return native; }
		operator bool() const { return IsValid(); }

		auto& GetNative() { return *native.Get(); }
		EQueueType GetType() const { return type; }

		void AddPendingContext(CommandContext& cmdCtx);
		GpuSync Submit();
		void SyncWith(GpuSync& sync);
		GpuSync Flush();

	private:
		CommandQueue(ComPtr<ID3D12CommandQueue> newNativeQueue, const EQueueType specifiedType, ComPtr<ID3D12Fence> newFence);

		GpuSync FlushUnsafe();

	private:
		static constexpr size_t RecommendedMinNumCommandContexts = 15;

	private:
		ComPtr<ID3D12CommandQueue> native;
		const EQueueType type;

		SharedMutex mutex;
		ComPtr<ID3D12Fence> fence;
		uint64_t syncCounter = 0;
		std::vector<std::reference_wrapper<CommandContext>> pendingContexts;
	};
} // namespace fe