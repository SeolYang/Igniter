#include <D3D12/GpuBuffer.h>
#include <D3D12/Device.h>

namespace fe::dx
{
	GpuBuffer::GpuBuffer(const GpuBufferDesc& newDesc, ComPtr<D3D12MA::Allocation> newAllocation,
						 ComPtr<ID3D12Resource> newResource)
		: desc(newDesc),
		  allocation(std::move(newAllocation)),
		  resource(std::move(newResource))
	{
	}

	GpuBuffer::GpuBuffer(GpuBuffer&& other) noexcept
		: desc(other.desc),
		  allocation(std::move(other.allocation)),
		  resource(std::move(other.resource))
	{
	}

	GpuBuffer::GpuBuffer(ComPtr<ID3D12Resource> bufferResource)
		: resource(std::move(bufferResource))
	{
		check(resource);
		desc.From(resource->GetDesc());
		check(desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER);
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

		return {
			mappedPtr, [this](uint8_t* ptr) { if (ptr != nullptr) Unmap(); }
		};
	}

	Handle<MappedGpuBuffer, GpuBuffer*> GpuBuffer::MapHandle(HandleManager& handleManager, const uint64_t offset)
	{
		uint8_t* mappedPtr = Map(offset);
		if (mappedPtr != nullptr)
		{
			return Handle<MappedGpuBuffer, GpuBuffer*>{
				handleManager, this,
				MappedGpuBuffer{ mappedPtr }
			};
		}

		checkNoEntry();
		return {};
	}

	void GpuBuffer::operator()(details::HandleImpl handle, const uint64_t evaluatedTypeHash, MappedGpuBuffer* mappedGPUBuffer)
	{
		verify(mappedGPUBuffer != nullptr);
		handle.Deallocate(evaluatedTypeHash);
	}

} // namespace fe::dx