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
		CommandContext(const Device& device, const String debugName, const D3D12_COMMAND_LIST_TYPE type);
		~CommandContext() = default;

		// Equivalent to Reset
		void Begin(const PipelineState* initState = nullptr);
		// Equivalent to Close
		void End();

		D3D12_COMMAND_LIST_TYPE GetType() const { return typeOfCommandList; }
		ID3D12GraphicsCommandList1& GetNative() const { return *cmdList.Get(); }

	private:
		ComPtr<ID3D12CommandAllocator> cmdAllocator;
		ComPtr<ID3D12GraphicsCommandList1> cmdList;
		const D3D12_COMMAND_LIST_TYPE			typeOfCommandList;

	};
} // namespace fe