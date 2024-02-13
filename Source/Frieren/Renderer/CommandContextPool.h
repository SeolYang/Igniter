#pragma once
#include <Renderer/Common.h>
#include <Core/FrameManager.h>
#include <Core/FrameResource.h>

namespace fe::dx
{
	class CommandContext;
}

namespace fe
{
	class CommandContextPool
	{
	public:
		FrameResource<dx::CommandContext> Request();

	};
} // namespace fe