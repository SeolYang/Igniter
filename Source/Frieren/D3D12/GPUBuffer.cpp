#include <D3D12/GPUBuffer.h>
#include <D3D12/Device.h>

namespace fe
{
	GPUBuffer::GPUBuffer(GPUBuffer&& other) noexcept
		: desc(other.desc), allocation(std::move(other.allocation))
	{
	}

	GPUBuffer::GPUBuffer(Device& device, const GPUBufferDesc& desc)
		: desc(desc), allocation(device.AllocateResource(desc.ToAllocationDesc(), desc))
	{
	}

	GPUBuffer::GPUBuffer(wrl::ComPtr<ID3D12Resource> existBuffer) : allocation(GPUResource{existBuffer})
	{
		FE_ASSERT(existBuffer.Get() != nullptr);
		desc.From(existBuffer->GetDesc());
	}

	GPUBuffer& GPUBuffer::operator=(GPUBuffer&& other) noexcept
	{
		this->desc = other.desc;
		this->allocation = std::move(other.allocation);
		return *this;
	}
} // namespace fe