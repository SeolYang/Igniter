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

		D3D12_COMMAND_LIST_TYPE GetType() const { return typeOfCommandList; }
		auto&					GetNative() { return *cmdList.Get(); }

	private:
		ComPtr<ID3D12CommandAllocator>	   cmdAllocator;
		ComPtr<ID3D12GraphicsCommandList7> cmdList;
		const D3D12_COMMAND_LIST_TYPE	   typeOfCommandList;
	};
} // namespace fe::dx