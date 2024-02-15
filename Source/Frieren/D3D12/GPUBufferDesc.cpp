#include <Core/Utility.h>
#include <D3D12/GPUBufferDesc.h>

namespace fe::dx
{
	void GPUBufferDesc::AsConstantBuffer(const uint32_t sizeOfBufferInBytes)
	{
		verify(sizeOfBufferInBytes > 0);
		bIsShaderReadWritable = false;
		bIsCPUAccessible = true;
		standardBarrier = { .SyncBefore = D3D12_BARRIER_SYNC_NONE,
							.SyncAfter = D3D12_BARRIER_SYNC_ALL_SHADING,
							.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS,
							.AccessAfter = D3D12_BARRIER_ACCESS_CONSTANT_BUFFER };

		structureByteStride = sizeOfBufferInBytes;
		numElements = 1;

		Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		Alignment = 0;
		Width = AdjustSizeForConstantBuffer(sizeOfBufferInBytes);
		Height = 1;
		DepthOrArraySize = 1;
		MipLevels = 1;
		SampleDesc = { .Count = 1, .Quality = 0 };
		Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		Flags = D3D12_RESOURCE_FLAG_NONE;
	}

	void GPUBufferDesc::AsStructuredBuffer(const uint32_t sizeOfElementInBytes, const uint32_t numOfElements,
										   const bool bEnableShaderReadWrtie /*= false*/)
	{
		verify(sizeOfElementInBytes > 0);
		verify(numOfElements > 0);
		bIsShaderReadWritable = bEnableShaderReadWrtie;
		bIsCPUAccessible = false;
		standardBarrier = { .SyncBefore = D3D12_BARRIER_SYNC_NONE,
							.SyncAfter = D3D12_BARRIER_SYNC_ALL_SHADING,
							.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS,
							.AccessAfter = bIsShaderReadWritable ? (D3D12_BARRIER_ACCESS_SHADER_RESOURCE |
																	D3D12_BARRIER_ACCESS_UNORDERED_ACCESS)
																 : D3D12_BARRIER_ACCESS_SHADER_RESOURCE };

		structureByteStride = sizeOfElementInBytes;
		numElements = numOfElements;

		Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		Alignment = 0;
		Width = sizeOfElementInBytes * numOfElements;
		Height = 1;
		DepthOrArraySize = 1;
		MipLevels = 1;
		SampleDesc = { .Count = 1, .Quality = 0 };
		Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		Flags = bIsShaderReadWritable ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
	}

	void GPUBufferDesc::AsUploadBuffer(const uint32_t sizeOfBufferInBytes)
	{
		verify(sizeOfBufferInBytes > 0);
		bIsShaderReadWritable = false;
		bIsCPUAccessible = true;
		standardBarrier = { .SyncBefore = D3D12_BARRIER_SYNC_NONE,
							.SyncAfter = D3D12_BARRIER_SYNC_COPY,
							.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS,
							.AccessAfter = D3D12_BARRIER_ACCESS_COPY_SOURCE };

		structureByteStride = sizeOfBufferInBytes;
		numElements = 1;

		Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		Alignment = 0;
		Width = sizeOfBufferInBytes;
		Height = 1;
		DepthOrArraySize = 1;
		MipLevels = 1;
		SampleDesc = { .Count = 1, .Quality = 0 };
		Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		Flags = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
	}

	void GPUBufferDesc::AsReadbackBuffer(const uint32_t sizeOfBufferInBytes)
	{
		verify(sizeOfBufferInBytes > 0);
		bIsShaderReadWritable = false;
		bIsCPUAccessible = true;
		standardBarrier = { .SyncBefore = D3D12_BARRIER_SYNC_NONE,
							.SyncAfter = D3D12_BARRIER_SYNC_RESOLVE,
							.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS,
							.AccessAfter = D3D12_BARRIER_ACCESS_RESOLVE_SOURCE };

		structureByteStride = sizeOfBufferInBytes;
		numElements = 1;

		Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		Alignment = 0;
		Width = sizeOfBufferInBytes;
		Height = 1;
		DepthOrArraySize = 1;
		MipLevels = 1;
		SampleDesc = { .Count = 1, .Quality = 0 };
		Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		Flags = D3D12_RESOURCE_FLAG_NONE;
	}

	void GPUBufferDesc::AsVertexBuffer(const uint32_t sizeOfVertexInBytes, const uint32_t numVertices)
	{
		verify(sizeOfVertexInBytes > 0);
		verify(numVertices > 0);
		bIsShaderReadWritable = false;
		bIsCPUAccessible = false;
		standardBarrier = { .SyncBefore = D3D12_BARRIER_SYNC_NONE,
							.SyncAfter = D3D12_BARRIER_SYNC_ALL_SHADING,
							.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS,
							.AccessAfter = D3D12_BARRIER_ACCESS_VERTEX_BUFFER };

		structureByteStride = sizeOfVertexInBytes;
		numElements = numVertices;

		Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		Alignment = 0;
		Width = sizeOfVertexInBytes * numVertices;
		Height = 1;
		DepthOrArraySize = 1;
		MipLevels = 1;
		SampleDesc = { .Count = 1, .Quality = 0 };
		Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		Flags = D3D12_RESOURCE_FLAG_NONE;
	}

	void GPUBufferDesc::AsIndexBuffer(const uint32_t sizeOfIndexInBytes, const uint32_t numIndices)
	{
		verify(sizeOfIndexInBytes > 0);
		verify(numIndices > 0);
		bIsShaderReadWritable = false;
		bIsCPUAccessible = false;
		standardBarrier = { .SyncBefore = D3D12_BARRIER_SYNC_NONE,
							.SyncAfter = D3D12_BARRIER_SYNC_INDEX_INPUT,
							.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS,
							.AccessAfter = D3D12_BARRIER_ACCESS_INDEX_BUFFER };

		structureByteStride = sizeOfIndexInBytes;
		numElements = numIndices;

		Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		Alignment = 0;
		Width = sizeOfIndexInBytes * numIndices;
		Height = 1;
		DepthOrArraySize = 1;
		MipLevels = 1;
		SampleDesc = { .Count = 1, .Quality = 0 };
		Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		Flags = D3D12_RESOURCE_FLAG_NONE;
	}

	bool GPUBufferDesc::IsConstantBufferCompatible() const
	{
		return BitFlagContains(standardBarrier.AccessAfter, D3D12_BARRIER_ACCESS_CONSTANT_BUFFER);
	}

	bool GPUBufferDesc::IsShaderResourceCompatible() const
	{
		return BitFlagContains(standardBarrier.AccessAfter, D3D12_BARRIER_ACCESS_SHADER_RESOURCE);
	}

	bool GPUBufferDesc::IsUnorderedAccessCompatible() const
	{
		return BitFlagContains(standardBarrier.AccessAfter, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS);
	}

	bool GPUBufferDesc::IsUploadCompatible() const
	{
		return bIsCPUAccessible && BitFlagContains(standardBarrier.AccessAfter, D3D12_BARRIER_ACCESS_COPY_SOURCE);
	}

	bool GPUBufferDesc::IsReadbackCompatible() const
	{
		return bIsCPUAccessible && BitFlagContains(standardBarrier.AccessAfter, D3D12_BARRIER_ACCESS_RESOLVE_SOURCE);
	}

	D3D12MA::ALLOCATION_DESC GPUBufferDesc::ToAllocationDesc() const
	{
		D3D12MA::ALLOCATION_DESC desc{ .HeapType = D3D12_HEAP_TYPE_DEFAULT };
		if (IsUploadCompatible())
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
		if (BitFlagContains(standardBarrier.AccessAfter, D3D12_BARRIER_ACCESS_SHADER_RESOURCE))
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			desc.Buffer.FirstElement = 0;
			desc.Buffer.NumElements = numElements;
			desc.Buffer.StructureByteStride = structureByteStride;
			return desc;
		}

		return std::nullopt;
	}

	std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC>
	GPUBufferDesc::ToConstantBufferViewDesc(const D3D12_GPU_VIRTUAL_ADDRESS bufferLocation) const
	{
		if (BitFlagContains(standardBarrier.AccessAfter, D3D12_BARRIER_ACCESS_CONSTANT_BUFFER))
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC desc{};
			desc.BufferLocation = bufferLocation;
			desc.SizeInBytes = static_cast<uint32_t>(Width);
			return desc;
		}

		return std::nullopt;
	}

	std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> GPUBufferDesc::ToUnorderedAccessViewDesc() const
	{
		if (BitFlagContains(standardBarrier.AccessAfter, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS))
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			desc.Buffer.FirstElement = 0;
			desc.Buffer.NumElements = numElements;
			desc.Buffer.StructureByteStride = structureByteStride;
			desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
			return desc;
		}

		return std::nullopt;
	}

	void GPUBufferDesc::From(const D3D12_RESOURCE_DESC& desc)
	{
		verify(desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER);
		Dimension = desc.Dimension;
		Alignment = desc.Alignment;
		Width = desc.Width;
		Height = desc.Height;
		DepthOrArraySize = desc.DepthOrArraySize;
		MipLevels = desc.MipLevels;
		Format = desc.Format;
		SampleDesc = desc.SampleDesc;
		Layout = desc.Layout;
		Flags = desc.Flags;
		SamplerFeedbackMipRegion = {};
	}
} // namespace fe::dx