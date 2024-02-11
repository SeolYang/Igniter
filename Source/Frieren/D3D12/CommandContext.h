#pragma once
#include <D3D12/Common.h>
#include <Core/String.h>

namespace fe::dx
{
	class Device;
	class PipelineState;
	class CommandContext
	{
		friend class Device;

	public:
		CommandContext(const CommandContext&) = delete;
		CommandContext(CommandContext&& other) noexcept;
		~CommandContext() = default;

		CommandContext& operator=(const CommandContext&) = delete;
		CommandContext& operator=(CommandContext&& other) noexcept;

		// Equivalent to Reset
		void Begin(PipelineState* const initState = nullptr);
		// Equivalent to Close
		void End();

		void AddGlobalBarrier(const D3D12_GLOBAL_BARRIER& barrierToPending);
		void AddTextureBarrier(const D3D12_TEXTURE_BARRIER& barrierToPending);

	private:
		CommandContext(ComPtr<ID3D12CommandAllocator> newCmdAllocator, ComPtr<ID3D12GraphicsCommandList7> newCmdList,
					   const D3D12_COMMAND_LIST_TYPE targetQueueType);

	private:
		ComPtr<ID3D12CommandAllocator>	   cmdAllocator;
		ComPtr<ID3D12GraphicsCommandList7> cmdList;
		D3D12_COMMAND_LIST_TYPE			   cmdListTargetQueueType;

		std::vector<D3D12_GLOBAL_BARRIER>  pendingGlobalBarriers;
		std::vector<D3D12_TEXTURE_BARRIER> pendingTextureBarriers;
		std::vector<D3D12_BUFFER_BARRIER>  pendingBufferBarriers;
	};
} // namespace fe::dx