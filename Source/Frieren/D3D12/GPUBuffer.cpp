#include <D3D12/GPUBuffer.h>

namespace fe
{
	GPUBuffer::GPUBuffer(const GPUBufferDesc& desc, GPUAllocation allocation)
		: desc(desc), allocation(std::move(allocation))
	{
	}

	GPUBuffer::GPUBuffer(GPUBuffer&& other) noexcept
		: desc(other.desc), allocation(std::move(other.allocation))
	{
	}

	GPUBuffer& GPUBuffer::operator=(GPUBuffer&& other) noexcept
	{
		this->desc = other.desc;
		this->allocation = std::move(other.allocation);
		return *this;
	}

	GPUBuffer::~GPUBuffer()
	{
		allocation.SafeRelease();
	}
} // namespace fe