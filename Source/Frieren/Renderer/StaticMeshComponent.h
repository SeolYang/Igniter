#pragma once
#include <Core/Handle.h>

namespace fe::dx
{
	class GPUBuffer;
	class GPUView;
}

namespace fe
{
	struct StaticMeshComponent
	{
	public:
		WeakHandle<dx::GPUView>	  VerticesBufferSRV = {};
		WeakHandle<dx::GPUBuffer> IndexBufferHandle = {};
		uint32_t				  NumIndices = 0;
	};
} // namespace fe