#pragma once
#include <memory>

namespace fe::dx
{
	class Device;
	class Fence;
}

namespace fe
{
	class FrameResource
	{
	public:
		FrameResource(dx::Device& device, const size_t numInflightFrames);
		~FrameResource();

		void BeginFrame(const size_t currentGlobalFrameIdx);

		size_t GetGlobalFrameIndex() const { return globalFrameIdx; }
		size_t GetLocalFrameIndex() const { return localFrameIdx; }

	private:
		const size_t numInflightFrames;
		size_t		 globalFrameIdx = 0;
		size_t		 localFrameIdx = 0;

		std::unique_ptr<dx::Fence> fence;

	};
} // namespace fe