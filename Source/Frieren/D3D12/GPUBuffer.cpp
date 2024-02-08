#include <D3D12/GPUBuffer.h>
#include <D3D12/Device.h>

namespace fe::dx
{
	GPUBuffer::GPUBuffer(const GPUBufferDesc& newDesc, ComPtr<D3D12MA::Allocation> newAllocation,
						 ComPtr<ID3D12Resource> newResource)
		: desc(newDesc), allocation(std::move(newAllocation)), resource(std::move(newResource))
	{
	}

	GPUBuffer::GPUBuffer(GPUBuffer&& other) noexcept
		: desc(other.desc), allocation(std::move(other.allocation)), resource(std::move(other.resource))
	{
	}

	GPUBuffer::GPUBuffer(ComPtr<ID3D12Resource> bufferResource) : resource(std::move(bufferResource))
	{
		check(resource);
		desc.From(resource->GetDesc());
		check(desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER);
	}

	GPUBuffer& GPUBuffer::operator=(GPUBuffer&& other) noexcept
	{
		desc = other.desc;
		allocation = std::move(other.allocation);
		resource = std::move(other.resource);
		return *this;
	}
} // namespace fe::dx