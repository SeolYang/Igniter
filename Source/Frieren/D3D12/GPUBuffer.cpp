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

	uint8_t* GPUBuffer::Map(const uint64_t offset)
	{
		check(resource);
		check(offset < desc.GetSizeAsBytes());
		// #todo offset % desc.GetAlignment() == 0

		uint8_t* mappedPtr = nullptr;
		if (!SUCCEEDED(resource->Map(0, nullptr, reinterpret_cast<void**>(&mappedPtr))))
		{
			return nullptr;
		}

		return mappedPtr + offset;
	}

	void GPUBuffer::Unmap()
	{
		resource->Unmap(0, nullptr);
	}

	GPUResourceMapGuard GPUBuffer::MapGuard(const uint64_t offset)
	{
		uint8_t* mappedPtr = Map(offset);
		if (mappedPtr == nullptr)
		{
			return nullptr;
		}

		return {
			mappedPtr, [this](uint8_t* ptr) { if (ptr != nullptr) Unmap(); }
		};
	}

	UniqueHandle<MappedGPUBuffer, GPUBuffer*> GPUBuffer::MapHandle(HandleManager& handleManager, const uint64_t offset)
	{
		uint8_t* mappedPtr = Map(offset);
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