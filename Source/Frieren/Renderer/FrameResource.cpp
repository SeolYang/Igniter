#include <Renderer/FrameResource.h>
#include <D3D12/Device.h>

namespace fe
{
	FrameResource::FrameResource(dx::Device& device, const size_t numInflightFrames)
		: numInflightFrames(numInflightFrames)
		, fence(device.CreateFence("FrameFence").value())
	{
	}

	FrameResource::~FrameResource() {}

	void FrameResource::BeginFrame(const size_t currentGlobalFrameIdx)
	{
		check(globalFrameIdx < currentGlobalFrameIdx);
		globalFrameIdx = currentGlobalFrameIdx;
		localFrameIdx = currentGlobalFrameIdx % numInflightFrames;

		check(fence);
		fence.Next();
	}
} // namespace fe