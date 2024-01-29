#include <D3D12/GPUTexture.h>
#include <D3D12/Device.h>

namespace fe::dx
{
	GPUTexture::GPUTexture(Device& device, const GPUTextureDesc& desc)
		: desc(desc), allocation(device.AllocateResource(desc.ToAllocationDesc() , desc))
	{
	}

	GPUTexture::GPUTexture(ComPtr<ID3D12Resource> existTexture)
		: allocation(GPUResource{existTexture})
	{
		verify(existTexture.Get() != nullptr);
		desc.From(existTexture->GetDesc());
		verify(desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER && desc.Dimension != D3D12_RESOURCE_DIMENSION_UNKNOWN);
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