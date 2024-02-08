#pragma once
#include <memory>
#include <D3D12/Fence.h>

namespace fe::dx
{
	class Device;
} // namespace fe::dx

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
		dx::Fence	 fence;
	};
} // namespace fe