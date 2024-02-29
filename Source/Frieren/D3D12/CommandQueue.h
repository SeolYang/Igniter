#pragma once
#include <D3D12/Common.h>
#include <Core/Container.h>

namespace fe
{
	class RenderDevice;
	class Fence;
	class CommandContext;

	class CommandQueue
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

		void SignalTo(Fence& fence);
		void NextSignalTo(Fence& fence);
		void WaitFor(Fence& fence);

		void AddPendingContext(CommandContext& cmdCtx);
		void FlushPendingContexts();

		void FlushQueue(RenderDevice& device);

	private:
		CommandQueue(ComPtr<ID3D12CommandQueue> newNativeQueue, const EQueueType specifiedType);

	private:
		static constexpr size_t RecommendedMinNumCommandContexts = 15;

	private:
		ComPtr<ID3D12CommandQueue> native;
		const EQueueType type;

		std::vector<std::reference_wrapper<CommandContext>> pendingContexts;
	};
} // namespace fe