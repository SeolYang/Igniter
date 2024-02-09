#include <Renderer/FrameResource.h>
#include <D3D12/Device.h>
#include <D3D12/Fence.h>

namespace fe
{
	FrameResource::FrameResource(dx::Device& device, const size_t numInflightFrames)
		: numInflightFrames(numInflightFrames)
		, fence(std::make_unique<dx::Fence>(device.CreateFence("FrameFence").value()))
	{
	}

	FrameResource::~FrameResource() {}

	void FrameResource::BeginFrame(const size_t currentGlobalFrameIdx)
	{
		check(globalFrameIdx < currentGlobalFrameIdx);
		globalFrameIdx = currentGlobalFrameIdx;
		localFrameIdx = currentGlobalFrameIdx % numInflightFrames;

		check(fence);
		fence->Next();
	}
} // namespace fe