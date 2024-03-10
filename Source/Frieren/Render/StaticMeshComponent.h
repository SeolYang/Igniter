#pragma once
#include <Core/Handle.h>

namespace fe
{
	class GpuBuffer;
	class GpuView;
} // namespace fe

namespace fe
{
	struct StaticMeshComponent
	{
	public:
		RefHandle<GpuView> VerticesBufferSRV = {};
		RefHandle<GpuBuffer> IndexBufferHandle = {};
		uint32_t NumIndices = 0;
		RefHandle<GpuView> DiffuseTex{};
		RefHandle<GpuView> DiffuseTexSampler{};
	};
} // namespace fe