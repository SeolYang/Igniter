#include <Core/Utility.h>
#include <D3D12/GPUBufferDesc.h>

namespace fe
{
	GPUBufferDesc GPUBufferDesc::BuildConstantBuffer(const uint32_t sizeOfBufferInBytes)
	{
		return {
			.DebugName = String("ConstantBuffer"),
			.SizeInBytes = Private::AdjustSizeForConstantBuffer(sizeOfBufferInBytes),
			.bIsShaderReadWritable = false,
			.bIsCPUAccessible = true,
			.StandardBarrier = D3D12_BUFFER_BARRIER {
				.SyncBefore = D3D12_BARRIER_SYNC_NONE,
				.SyncAfter = D3D12_BARRIER_SYNC_ALL_SHADING,
				.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS,
				.AccessAfter = D3D12_BARRIER_ACCESS_CONSTANT_BUFFER }
		};
	}

	GPUBufferDesc GPUBufferDesc::BuildStructuredBuffer(const uint32_t sizeOfElementInBytes, const uint32_t numOfElements, const bool bIsDynamic)
	{
		D3D12_BARRIER_ACCESS access = D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
		if (bIsDynamic)
		{
			access |= D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
		}

		return {
			.DebugName = String("StructuredBuffer"),
			.SizeInBytes = sizeOfElementInBytes * numOfElements,
			.NumElements = numOfElements,
			.ElementStrideInBytes = sizeOfElementInBytes,
			.bIsShaderReadWritable = bIsDynamic,
			.bIsCPUAccessible = false,
			.StandardBarrier = D3D12_BUFFER_BARRIER {
				.SyncBefore = D3D12_BARRIER_SYNC_NONE,
				.SyncAfter = D3D12_BARRIER_SYNC_ALL_SHADING,
				.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS,
				.AccessAfter = access }
		};
	}

	GPUBufferDesc GPUBufferDesc::BuildUploadBuffer(const uint32_t sizeOfBufferInBytes)
	{
		return {
			.DebugName = String("UploadBuffer"),
			.SizeInBytes = sizeOfBufferInBytes,
			.bIsShaderReadWritable = false,
			.bIsCPUAccessible = true,
			.StandardBarrier = D3D12_BUFFER_BARRIER {
				.SyncBefore = D3D12_BARRIER_SYNC_NONE,
				.SyncAfter = D3D12_BARRIER_SYNC_COPY,
				.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS,
				.AccessAfter = D3D12_BARRIER_ACCESS_COPY_SOURCE }
		};
	}

	fe::GPUBufferDesc GPUBufferDesc::BuildReadbackBuffer(const uint32_t sizeOfBufferInBytes)
	{
		return {
			.DebugName = String("ReadbackBuffer"),
			.SizeInBytes = sizeOfBufferInBytes,
			.bIsShaderReadWritable = false,
			.bIsCPUAccessible = true,
			.StandardBarrier = D3D12_BUFFER_BARRIER {
				.SyncBefore = D3D12_BARRIER_SYNC_NONE,
				.SyncAfter = D3D12_BARRIER_SYNC_RESOLVE,
				.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS,
				.AccessAfter = D3D12_BARRIER_ACCESS_RESOLVE_SOURCE }
		};
	}

	GPUBufferDesc GPUBufferDesc::BuildVertexBuffer(const uint32_t sizeOfVertexInBytes, const uint32_t numVertices)
	{
		return {
			.DebugName = String("VertexBuffer"),
			.SizeInBytes = sizeOfVertexInBytes * numVertices,
			.NumElements = numVertices,
			.ElementStrideInBytes = sizeOfVertexInBytes,
			.bIsShaderReadWritable = false,
			.bIsCPUAccessible = false,
			.StandardBarrier = D3D12_BUFFER_BARRIER {
				.SyncBefore = D3D12_BARRIER_SYNC_NONE,
				.SyncAfter = D3D12_BARRIER_SYNC_ALL_SHADING,
				.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS,
				.AccessAfter = D3D12_BARRIER_ACCESS_VERTEX_BUFFER }
		};
	}

	GPUBufferDesc GPUBufferDesc::BuildIndexBuffer(const uint32_t sizeOfIndexInBytes, const uint32_t numIndices)
	{
		return {
			.DebugName = String("IndexBuffer"),
			.SizeInBytes = sizeOfIndexInBytes * numIndices,
			.NumElements = numIndices,
			.ElementStrideInBytes = sizeOfIndexInBytes,
			.bIsShaderReadWritable = false,
			.bIsCPUAccessible = false,
			.StandardBarrier = D3D12_BUFFER_BARRIER {
				.SyncBefore = D3D12_BARRIER_SYNC_NONE,
				.SyncAfter = D3D12_BARRIER_SYNC_INDEX_INPUT,
				.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS,
				.AccessAfter = D3D12_BARRIER_ACCESS_INDEX_BUFFER }
		};
	}

	bool GPUBufferDesc::IsConstantBufferCompatible() const
	{
		return BitFlagContains(StandardBarrier.AccessAfter, D3D12_BARRIER_ACCESS_CONSTANT_BUFFER);
	}

	bool GPUBufferDesc::IsShaderResourceCompatible() const
	{
		return BitFlagContains(StandardBarrier.AccessAfter, D3D12_BARRIER_ACCESS_SHADER_RESOURCE);
	}

	bool GPUBufferDesc::IsUnorderedAccessCompatible() const
	{
		return BitFlagContains(StandardBarrier.AccessAfter, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS);
	}

	bool GPUBufferDesc::IsUploadCompatible() const
	{
		return bIsCPUAccessible && BitFlagContains(StandardBarrier.AccessAfter, D3D12_BARRIER_ACCESS_COPY_SOURCE);
	}

	bool GPUBufferDesc::IsReadbackCompatible() const
	{
		return bIsCPUAccessible && BitFlagContains(StandardBarrier.AccessAfter, D3D12_BARRIER_ACCESS_RESOLVE_SOURCE);
	}

	D3D12_RESOURCE_DESC1 GPUBufferDesc::ToResourceDesc() const
	{
		D3D12_RESOURCE_DESC1 desc {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		/** resource alignment: https://asawicki.info/news_1726_secrets_of_direct3d_12_resource_alignment */
		desc.Alignment = 0;
		desc.Width = SizeInBytes;
		desc.Height = 1;
		desc.MipLevels = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.SampleDesc = { .Count = 1, .Quality = 0 };
		desc.Flags = bIsShaderReadWritable ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		return desc;
	}

	D3D12MA::ALLOCATION_DESC GPUBufferDesc::ToAllocationDesc() const
	{
		D3D12MA::ALLOCATION_DESC desc {};
		if (IsShaderResourceCompatible() || IsConstantBufferCompatible() || IsUnorderedAccessCompatible())
		{
			desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
		}
		else if (IsUploadCompatible())
		{
			desc.HeapType = bIsCPUAccessible ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_GPU_UPLOAD;
		}
		else if (IsReadbackCompatible())
		{
			desc.HeapType = D3D12_HEAP_TYPE_READBACK;
		}

		return desc;
	}

	std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> GPUBufferDesc::ToShaderResourceViewDesc() const
	{
		if (BitFlagContains(StandardBarrier.AccessAfter, D3D12_BARRIER_ACCESS_SHADER_RESOURCE))
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC desc {};
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

	std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC> GPUBufferDesc::ToConstantBufferViewDesc(const D3D12_GPU_VIRTUAL_ADDRESS bufferLocation) const
	{
		if (BitFlagContains(StandardBarrier.AccessAfter, D3D12_BARRIER_ACCESS_CONSTANT_BUFFER))
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC desc {};
			desc.BufferLocation = bufferLocation;
			desc.SizeInBytes = SizeInBytes;
			return desc;
		}

		return std::nullopt;
	}

	std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> GPUBufferDesc::ToUnorderedAccessViewDesc() const
	{
		if (BitFlagContains(StandardBarrier.AccessAfter, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS))
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC desc {};
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