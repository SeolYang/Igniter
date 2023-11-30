#include <D3D12/GPUBuffer.h>

namespace fe
{
	GPUBuffer::GPUBuffer(const GPUBufferDescription description, const Private::GPUAllocation allocation, std::optional<Descriptor> cbv, std::optional<Descriptor> srv, std::optional<Descriptor> uav)
		: description(description), allocation(allocation), srv(std::move(srv)), cbv(std::move(cbv)), uav(std::move(uav))
	{
	}

	GPUBuffer::~GPUBuffer()
	{
		allocation.Resource->Release();
		allocation.Allocation->Release();
	}
} // namespace fe