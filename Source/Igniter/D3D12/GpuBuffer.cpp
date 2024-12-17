#include "Igniter/Igniter.h"
#include "Igniter/D3D12/GpuBuffer.h"

namespace ig
{
    GpuBuffer::GpuBuffer(const GpuBufferDesc& newDesc, ComPtr<D3D12MA::Allocation> newAllocation, ComPtr<ID3D12Resource> newResource) : 
        desc(newDesc),
        allocation(std::move(newAllocation)),
        resource(std::move(newResource))
    {
    }

    GpuBuffer::GpuBuffer(GpuBuffer&& other) noexcept : 
        desc(other.desc),
        allocation(std::move(other.allocation)), 
        resource(std::move(other.resource))
    {
    }

    GpuBuffer::GpuBuffer(ComPtr<ID3D12Resource> bufferResource) : resource(std::move(bufferResource))
    {
        IG_CHECK(resource);
        desc.From(resource->GetDesc());
        IG_CHECK(desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER);
    }

    GpuBuffer& GpuBuffer::operator=(GpuBuffer&& other) noexcept
    {
        desc = other.desc;
        allocation = std::move(other.allocation);
        resource = std::move(other.resource);
        return *this;
    }

    uint8_t* GpuBuffer::Map(const uint64_t offset)
    {
        IG_CHECK(resource);
        IG_CHECK(offset < desc.GetSizeAsBytes());

        uint8_t* mappedPtr = nullptr;
        if (!SUCCEEDED(resource->Map(0, nullptr, reinterpret_cast<void**>(&mappedPtr))))
        {
            return nullptr;
        }

        return mappedPtr + offset;
    }

    void GpuBuffer::Unmap()
    {
        resource->Unmap(0, nullptr);
    }

    GPUResourceMapGuard GpuBuffer::MapGuard(const uint64_t offset)
    {
        uint8_t* mappedPtr = Map(offset);
        if (mappedPtr == nullptr)
        {
            return nullptr;
        }

        return { mappedPtr, [this](uint8_t* ptr)
            {
                if (ptr != nullptr)
                    Unmap();
            } };
    }
}    // namespace ig
