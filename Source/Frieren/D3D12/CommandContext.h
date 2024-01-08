#pragma once
#include <D3D12/Common.h>
#include <Core/String.h>

namespace fe
{
	class Device;
    class CommandContext
    {
	public:
		CommandContext(Device& device, const String debugName, const D3D12_COMMAND_LIST_TYPE type);

    };
}