#pragma once
#include <Core/FrameResource.h>
#include <Core/Handle.h>

namespace fe::dx
{
	class GPUBuffer;
}

namespace fe
{
	// #idea Handle 의 확장을 통한 Deferred Deallocation 은?
	struct StaticMeshComponent
	{
	public:
		WeakHandle<dx::GPUBuffer> VertexBufferHandle = {};
		WeakHandle<dx::GPUBuffer> IndexBufferHandle = {};
		uint32_t				  NumIndices = 0;
	};
} // namespace fe