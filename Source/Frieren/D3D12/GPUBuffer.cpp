#include <D3D12/GPUBuffer.h>
#include <D3D12/Device.h>

namespace fe::dx
{
	GPUBuffer::GPUBuffer(GPUBuffer&& other) noexcept : desc(other.desc), allocation(std::move(other.allocation)) {}

	GPUBuffer::GPUBuffer(Device& device, const GPUBufferDesc& desc)
		: desc(desc), allocation(device.AllocateResource(desc.ToAllocationDesc(), desc))
	{
	}

	GPUBuffer::GPUBuffer(ComPtr<ID3D12Resource> existBuffer) : allocation(GPUResource{ existBuffer })
	{
		check(existBuffer.Get() != nullptr);
		desc.From(existBuffer->GetDesc());
		check(desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER);
	}

	GPUBuffer& GPUBuffer::operator=(GPUBuffer&& other) noexcept
	{
		this->desc = other.desc;
		this->allocation = std::move(other.allocation);
		return *this;
	}
} // namespace fe::dx