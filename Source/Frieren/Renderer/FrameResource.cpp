#include <Renderer/FrameResource.h>
#include <D3D12/Device.h>
#include <D3D12/Fence.h>

namespace fe
{
	FrameResource::FrameResource(const dx::Device& device, const size_t numInflightFrames)
		: numInflightFrames(numInflightFrames), fence(std::make_unique<dx::Fence>(device, "FrameFence"))
	{
	}

	FrameResource::~FrameResource()
	{
		fence.reset();
	}

	void FrameResource::BeginFrame(const size_t currentGlobalFrameIdx)
	{
		check(globalFrameIdx < currentGlobalFrameIdx);
		globalFrameIdx = currentGlobalFrameIdx;
		localFrameIdx = currentGlobalFrameIdx % numInflightFrames;

		check(fence != nullptr);
		fence->Next();
	}
} // namespace fe