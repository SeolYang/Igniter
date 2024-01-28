#pragma once
#include <memory>

namespace fe
{
	class Fence;
    class FrameResource
    {
	private:
		size_t localFrameIndex;
		size_t globalFrameIndex;

        // Local Frame Index
        // Global Frame Index
        // Back buffer
        // Frame Fence

		//std::unique_ptr<Fence> fence;

    };
}