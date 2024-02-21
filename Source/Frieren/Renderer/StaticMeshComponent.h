#pragma once
#include <Core/FrameResource.h>
#include <Core/Handle.h>

namespace fe::dx
{
	class GPUBuffer;
}

namespace fe
{
	struct StaticMeshComponent
	{
	public:
		WeakHandle<dx::GPUBuffer> VertexBufferHandle = {};
		WeakHandle<dx::GPUBuffer> IndexBufferHandle = {};
		uint32_t				  NumIndices = 0;
	};
} // namespace fe