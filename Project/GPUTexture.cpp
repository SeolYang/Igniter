#include <D3D12/GPUTexture.h>

namespace fe
{
	GPUTexture::~GPUTexture()
	{
	}

	GPUTexture::GPUTexture(GPUTexture&& other) noexcept
		: desc(other.desc), allocation(std::move(other.allocation))
	{
	}

	GPUTexture::GPUTexture(const GPUTextureDesc& desc, GPUAllocation allocation)
		: desc(desc), allocation(std::move(allocation))
	{
	}

	GPUTexture& GPUTexture::operator=(GPUTexture&& other) noexcept
	{
		this->desc = other.desc;
		this->allocation = std::move(other.allocation);
		return *this;
	}
} // namespace fe