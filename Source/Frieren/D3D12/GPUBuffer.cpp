#include <D3D12/GPUBuffer.h>
#include <D3D12/Device.h>

namespace fe::dx
{
	GPUBuffer::GPUBuffer(const GPUBufferDesc& newDesc, ComPtr<D3D12MA::Allocation> newAllocation,
						 ComPtr<ID3D12Resource> newResource)
		: desc(newDesc),
		  allocation(std::move(newAllocation)),
		  resource(std::move(newResource))
	{
	}

	GPUBuffer::GPUBuffer(GPUBuffer&& other) noexcept
		: desc(other.desc),
		  allocation(std::move(other.allocation)),
		  resource(std::move(other.resource))
	{
	}

	GPUBuffer::GPUBuffer(ComPtr<ID3D12Resource> bufferResource)
		: resource(std::move(bufferResource))
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

	uint8_t* GPUBuffer::Map(const uint32_t subresource /*= 0*/, const CD3DX12_RANGE range /*= {0, 0}*/)
	{
		check(resource);
		uint8_t* mappedPtr = nullptr;
		if (!SUCCEEDED(resource->Map(subresource, &range, reinterpret_cast<void**>(&mappedPtr))))
		{
			return nullptr;
		}

		return mappedPtr;
	}

	void GPUBuffer::Unmap()
	{
		resource->Unmap(0, nullptr);
	}

	GPUResourceMapGuard GPUBuffer::MapGuard(const uint32_t subresource /*= 0*/, const CD3DX12_RANGE range /*= {0, 0}*/)
	{
		uint8_t* mappedPtr = Map(subresource, range);
		if (mappedPtr == nullptr)
		{
			return nullptr;
		}

		return {
			mappedPtr, [this](uint8_t* /* ptr */) { Unmap(); }
		};
	}

	UniqueHandle<MappedGPUBuffer, GPUBuffer*> GPUBuffer::MapHandle(HandleManager& handleManager, const uint32_t subresource /* = 0 */, const CD3DX12_RANGE range /* = */)
	{
		uint8_t* mappedPtr = Map(subresource, range);
		if (mappedPtr != nullptr)
		{
			return UniqueHandle<MappedGPUBuffer, GPUBuffer*>{
				handleManager, this,
				MappedGPUBuffer{ mappedPtr }
			};
		}

		checkNoEntry();
		return {};
	}

	void GPUBuffer::operator()(Handle handle, const uint64_t evaluatedTypeHash, MappedGPUBuffer* mappedGPUBuffer)
	{
		check(mappedGPUBuffer != nullptr);
		handle.Deallocate(evaluatedTypeHash);
	}

} // namespace fe::dx