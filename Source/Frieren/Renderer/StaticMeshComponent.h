#pragma once
#include <Core/Handle.h>

namespace fe::dx
{
	class GpuBuffer;
	class GpuView;
}

namespace fe
{
	struct StaticMeshComponent
	{
	public:
		WeakHandle<dx::GpuView>	  VerticesBufferSRV = {};
		WeakHandle<dx::GpuBuffer> IndexBufferHandle = {};
		uint32_t				  NumIndices = 0;
	};
} // namespace fe