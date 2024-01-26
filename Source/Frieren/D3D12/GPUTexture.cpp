#include <D3D12/GPUTexture.h>
#include <D3D12/Device.h>

namespace fe
{
	GPUTexture::GPUTexture(Device& device, const GPUTextureDesc& desc)
		: desc(desc), allocation(device.AllocateResource(desc.ToResourceDesc(), desc.ToAllocationDesc()))
	{
	}

	GPUTexture::~GPUTexture()
	{
	}

	GPUTexture::GPUTexture(GPUTexture&& other) noexcept
		: desc(other.desc), allocation(std::move(other.allocation))
	{
	}

	GPUTexture& GPUTexture::operator=(GPUTexture&& other) noexcept
	{
		this->desc = other.desc;
		this->allocation = std::move(other.allocation);
		return *this;
	}
} // namespace fe