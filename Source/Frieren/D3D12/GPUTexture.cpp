#include <D3D12/GPUTexture.h>
#include <D3D12/Device.h>

namespace fe::dx
{
	GPUTexture::GPUTexture(const GPUTextureDesc& newDesc, ComPtr<D3D12MA::Allocation> newAllocation,
						   ComPtr<ID3D12Resource> newResource)
		: desc(newDesc), allocation(std::move(newAllocation)), resource(std::move(newResource))
	{
	}

	GPUTexture::GPUTexture(ComPtr<ID3D12Resource> textureResource) : resource(std::move(textureResource))
	{
		check(resource);
		desc.From(resource->GetDesc());
		check(desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER && desc.Dimension != D3D12_RESOURCE_DIMENSION_UNKNOWN);
	}

	GPUTexture::GPUTexture(GPUTexture&& other) noexcept
		: desc(other.desc), allocation(std::move(other.allocation)), resource(std::move(other.resource))
	{
	}

	GPUTexture& GPUTexture::operator=(GPUTexture&& other) noexcept
	{
		this->desc = other.desc;
		this->allocation = std::move(other.allocation);
		return *this;
	}
} // namespace fe::dx