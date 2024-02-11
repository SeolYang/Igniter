#pragma once
#include <memory>

namespace fe::dx
{
	class Device;
	class Fence;
} // namespace fe::dx

namespace fe
{
	class FrameResource
	{
	public:
		FrameResource(dx::Device& device);
		FrameResource(FrameResource&& other) noexcept;
		~FrameResource();

		FrameResource& operator=(FrameResource&& other) noexcept;

		dx::Fence& GetFence();

	private:
		std::unique_ptr<dx::Fence> fence;
	};
} // namespace fe