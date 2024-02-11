#include <Renderer/FrameResource.h>
#include <D3D12/Device.h>
#include <D3D12/Fence.h>

namespace fe
{
	FrameResource::FrameResource(dx::Device& device)
		: fence(std::make_unique<dx::Fence>(device.CreateFence("FrameFence").value()))
	{
	}

	FrameResource::FrameResource(FrameResource&& other) noexcept : fence(std::move(other.fence)) {}

	FrameResource::~FrameResource() {}

	FrameResource& FrameResource::operator=(FrameResource&& other) noexcept
	{
		fence = std::move(other.fence);
		return *this;
	}

	dx::Fence& FrameResource::GetFence()
	{
		check(fence);
		return *fence;
	}
} // namespace fe