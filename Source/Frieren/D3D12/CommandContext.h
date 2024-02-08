#pragma once
#include <D3D12/Common.h>
#include <Core/String.h>

namespace fe::dx
{
	class Device;
	class PipelineState;
	class CommandContext
	{
	public:
		CommandContext(Device& device, const String debugName, const D3D12_COMMAND_LIST_TYPE type);
		~CommandContext() = default;

		// Equivalent to Reset
		void Begin(PipelineState* const initState = nullptr);
		// Equivalent to Close
		void End();

		void AddGlobalBarrier(const D3D12_GLOBAL_BARRIER& barrierToPending);
		void AddTextureBarrier(const D3D12_TEXTURE_BARRIER& barrierToPending);

		D3D12_COMMAND_LIST_TYPE GetType() const { return typeOfCommandList; }

	private:
		ComPtr<ID3D12CommandAllocator>	   cmdAllocator;
		ComPtr<ID3D12GraphicsCommandList7> cmdList;
		const D3D12_COMMAND_LIST_TYPE	   typeOfCommandList;

		std::vector<D3D12_GLOBAL_BARRIER>  pendingGlobalBarriers;
		std::vector<D3D12_TEXTURE_BARRIER> pendingTextureBarriers;
		std::vector<D3D12_BUFFER_BARRIER>  pendingBufferBarriers;

	};
} // namespace fe::dx