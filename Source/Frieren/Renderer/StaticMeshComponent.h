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
		RefHandle<dx::GpuView>	  VerticesBufferSRV = {};
		RefHandle<dx::GpuBuffer> IndexBufferHandle = {};
		uint32_t				  NumIndices = 0;
	};
} // namespace fe