#include <D3D12/GpuTexture.h>
#include <D3D12/RenderDevice.h>

namespace ig
{
    GpuTexture::GpuTexture(const GPUTextureDesc& newDesc, ComPtr<D3D12MA::Allocation> newAllocation,
                           ComPtr<ID3D12Resource> newResource)
        : desc(newDesc),
          allocation(std::move(newAllocation)),
          resource(std::move(newResource))
    {
    }

    GpuTexture::GpuTexture(ComPtr<ID3D12Resource> textureResource)
        : resource(std::move(textureResource))
    {
        IG_CHECK(resource);
        desc.From(resource->GetDesc());
        IG_CHECK(desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER && desc.Dimension != D3D12_RESOURCE_DIMENSION_UNKNOWN);
    }

    GpuTexture::GpuTexture(GpuTexture&& other) noexcept
        : desc(other.desc),
          allocation(std::move(other.allocation)),
          resource(std::move(other.resource))
    {
    }

    GpuTexture& GpuTexture::operator=(GpuTexture&& other) noexcept
    {
        this->desc = other.desc;
        this->allocation = std::move(other.allocation);
        return *this;
    }
} // namespace ig