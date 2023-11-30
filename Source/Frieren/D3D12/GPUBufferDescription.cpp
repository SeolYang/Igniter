#include <D3D12/GPUBufferDescription.h>
#include <Core/Utility.h>

namespace fe
{
	GPUBufferDescription GPUBufferDescription::BuildConstantBufferDescription(const uint32_t sizeOfBufferInBytes)
	{
		return {
			.SizeInBytes = Private::AdjustSizeForConstantBuffer(sizeOfBufferInBytes),
			.bIsDynamic = false,
			.bIsCpuAccessible = true,
			.StandardBarrier = D3D12_BUFFER_BARRIER{
				.SyncBefore = D3D12_BARRIER_SYNC_NONE,
				.SyncAfter = D3D12_BARRIER_SYNC_ALL_SHADING,
				.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS,
				.AccessAfter = D3D12_BARRIER_ACCESS_CONSTANT_BUFFER }
		};
	}

	GPUBufferDescription GPUBufferDescription::BuildStructuredBufferDescription(const uint32_t sizeOfElementInBytes, const uint32_t numOfElements, const bool bIsDynamic)
	{
		D3D12_BARRIER_ACCESS access = D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
		if (bIsDynamic)
		{
			access |= D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
		}

		return {
			.SizeInBytes = sizeOfElementInBytes * numOfElements,
			.NumElements = numOfElements,
			.ElementStrideInBytes = sizeOfElementInBytes,
			.bIsDynamic = bIsDynamic,
			.bIsCpuAccessible = false,
			.StandardBarrier = D3D12_BUFFER_BARRIER{
				.SyncBefore = D3D12_BARRIER_SYNC_NONE,
				.SyncAfter = D3D12_BARRIER_SYNC_ALL_SHADING,
				.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS,
				.AccessAfter = access }
		};
	}

	GPUBufferDescription GPUBufferDescription::BuildUploadBufferDescription(const uint32_t sizeOfBufferInBytes)
	{
		return {
			.SizeInBytes = sizeOfBufferInBytes,
			.bIsDynamic = false,
			.bIsCpuAccessible = true,
			.StandardBarrier = D3D12_BUFFER_BARRIER{
				.SyncBefore = D3D12_BARRIER_SYNC_NONE,
				.SyncAfter = D3D12_BARRIER_SYNC_COPY,
				.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS,
				.AccessAfter = D3D12_BARRIER_ACCESS_COPY_SOURCE }
		};
	}

	fe::GPUBufferDescription GPUBufferDescription::BuildReadbackBufferDescription(const uint32_t sizeOfBufferInBytes)
	{
		return {
			.SizeInBytes = sizeOfBufferInBytes,
			.bIsDynamic = false,
			.bIsCpuAccessible = true,
			.StandardBarrier = D3D12_BUFFER_BARRIER{
				.SyncBefore = D3D12_BARRIER_SYNC_NONE,
				.SyncAfter = D3D12_BARRIER_SYNC_RESOLVE,
				.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS,
				.AccessAfter = D3D12_BARRIER_ACCESS_RESOLVE_SOURCE }
		};
	}

	GPUBufferDescription GPUBufferDescription::BuildVertexBufferDescription(const uint32_t sizeOfVertexInBytes, const uint32_t numVertices)
	{
		return {
			.SizeInBytes = sizeOfVertexInBytes * numVertices,
			.NumElements = numVertices,
			.ElementStrideInBytes = sizeOfVertexInBytes,
			.bIsDynamic = false,
			.bIsCpuAccessible = false,
			.StandardBarrier = D3D12_BUFFER_BARRIER{
				.SyncBefore = D3D12_BARRIER_SYNC_NONE,
				.SyncAfter = D3D12_BARRIER_SYNC_ALL_SHADING,
				.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS,
				.AccessAfter = D3D12_BARRIER_ACCESS_VERTEX_BUFFER }
		};
	}

	GPUBufferDescription GPUBufferDescription::BuildIndexBufferDescription(const uint32_t sizeOfIndexInBytes, const uint32_t numIndices)
	{
		return {
			.SizeInBytes = sizeOfIndexInBytes * numIndices,
			.NumElements = numIndices,
			.ElementStrideInBytes = sizeOfIndexInBytes,
			.bIsDynamic = false,
			.bIsCpuAccessible = false,
			.StandardBarrier = D3D12_BUFFER_BARRIER{
				.SyncBefore = D3D12_BARRIER_SYNC_NONE,
				.SyncAfter = D3D12_BARRIER_SYNC_INDEX_INPUT,
				.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS,
				.AccessAfter = D3D12_BARRIER_ACCESS_INDEX_BUFFER }
		};
	}

	bool GPUBufferDescription::IsConstantBufferCompatible() const
	{
		return BitFlagContains(StandardBarrier.AccessAfter, D3D12_BARRIER_ACCESS_CONSTANT_BUFFER);
	}

	bool GPUBufferDescription::IsShaderResourceCompatible() const
	{
		return BitFlagContains(StandardBarrier.AccessAfter, D3D12_BARRIER_ACCESS_SHADER_RESOURCE);
	}

	bool GPUBufferDescription::IsUnorderedAccessCompatible() const
	{
		return BitFlagContains(StandardBarrier.AccessAfter, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS);
	}

	bool GPUBufferDescription::IsUploadCompatible() const
	{
		return bIsCpuAccessible && BitFlagContains(StandardBarrier.AccessAfter, D3D12_BARRIER_ACCESS_COPY_SOURCE);
	}

	bool GPUBufferDescription::IsReadbackCompatible() const
	{
		return bIsCpuAccessible && BitFlagContains(StandardBarrier.AccessAfter, D3D12_BARRIER_ACCESS_RESOLVE_SOURCE);
	}

	D3D12_RESOURCE_DESC1 GPUBufferDescription::AsResourceDescription() const
	{
		D3D12_RESOURCE_DESC1 desc{};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		/** resource alignment: https://asawicki.info/news_1726_secrets_of_direct3d_12_resource_alignment */
		desc.Alignment = 0;
		desc.Width = SizeInBytes;
		desc.Height = 1;
		desc.MipLevels = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.SampleDesc = { .Count = 1, .Quality = 0 };
		desc.Flags = bIsDynamic ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		return desc;
	}

	D3D12MA::ALLOCATION_DESC GPUBufferDescription::AsAllocationDescription() const
	{
		D3D12MA::ALLOCATION_DESC desc{};
		if (IsShaderResourceCompatible() || IsConstantBufferCompatible() || IsUnorderedAccessCompatible())
		{
			desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
		}
		else if (IsUploadCompatible())
		{
			desc.HeapType = bIsCpuAccessible ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_GPU_UPLOAD;
		}
		else if (IsReadbackCompatible())
		{
			desc.HeapType = D3D12_HEAP_TYPE_READBACK;
		}

		return desc;
	}

	std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> GPUBufferDescription::AsShaderResourceViewDescription() const
	{
		if (BitFlagContains(StandardBarrier.AccessAfter, D3D12_BARRIER_ACCESS_SHADER_RESOURCE))
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			desc.Buffer.FirstElement = 0;
			desc.Buffer.NumElements = NumElements;
			desc.Buffer.StructureByteStride = ElementStrideInBytes;
			return desc;
		}

		return std::nullopt;
	}

	std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC> GPUBufferDescription::AsConstantBufferViewDescription(const D3D12_GPU_VIRTUAL_ADDRESS bufferLocation) const
	{
		if (BitFlagContains(StandardBarrier.AccessAfter, D3D12_BARRIER_ACCESS_CONSTANT_BUFFER))
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC desc{};
			desc.BufferLocation = bufferLocation;
			desc.SizeInBytes = SizeInBytes;
			return desc;
		}

		return std::nullopt;
	}

	std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> GPUBufferDescription::AsUnorderedAccessViewDescription() const
	{
		if (BitFlagContains(StandardBarrier.AccessAfter, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS))
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			desc.Buffer.FirstElement = 0;
			desc.Buffer.NumElements = NumElements;
			desc.Buffer.StructureByteStride = ElementStrideInBytes;
			desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
			return desc;
		}

		return std::nullopt;
	}
} // namespace fe