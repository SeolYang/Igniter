#include <Core/Container.h>
#include <D3D12/GPUBufferDesc.h>

namespace fe::dx
{
	void GPUBufferDesc::AsConstantBuffer(const uint32_t sizeOfBufferInBytes)
	{
		verify(sizeOfBufferInBytes > 0);
		bIsShaderReadWritable = false;
		bIsCPUAccessible = true;
		bufferType = EBufferType::ConstantBuffer;

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

	void GPUBufferDesc::AsStructuredBuffer(const uint32_t sizeOfElementInBytes, const uint32_t numOfElements, const bool bEnableShaderReadWrtie /*= false*/)
	{
		verify(sizeOfElementInBytes > 0);
		verify(numOfElements > 0);
		bIsShaderReadWritable = bEnableShaderReadWrtie;
		bIsCPUAccessible = false;
		bufferType = EBufferType::StructuredBuffer;

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
		bufferType = EBufferType::UploadBuffer;

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
		bufferType = EBufferType::ReadbackBuffer;

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
		bufferType = EBufferType::VertexBuffer;

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
		bufferType = EBufferType::IndexBuffer;

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

	D3D12MA::ALLOCATION_DESC GPUBufferDesc::ToAllocationDesc() const
	{
		check(bufferType != EBufferType::Unknown);
		D3D12MA::ALLOCATION_DESC desc{ .HeapType = D3D12_HEAP_TYPE_DEFAULT };
		switch (bufferType)
		{
			case EBufferType::UploadBuffer:
				desc.HeapType = bIsCPUAccessible ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_GPU_UPLOAD;
				break;
			case EBufferType::ReadbackBuffer:
				desc.HeapType = D3D12_HEAP_TYPE_READBACK;
				break;
		}

		return desc;
	}

	std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC> GPUBufferDesc::ToConstantBufferViewDesc(const D3D12_GPU_VIRTUAL_ADDRESS bufferLocation) const
	{
		check(bufferType != EBufferType::Unknown);

		std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC> desc{};
		if (IsConstantBufferViewCompatibleBuffer(bufferType))
		{
			desc = D3D12_CONSTANT_BUFFER_VIEW_DESC{
				.BufferLocation = bufferLocation,
				.SizeInBytes = static_cast<uint32_t>(Width)
			};
		}

		return desc;
	}

	std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> GPUBufferDesc::ToShaderResourceViewDesc() const
	{
		check(bufferType != EBufferType::Unknown);

		std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> desc{};
		if (IsShaderResourceViewCompatibleBuffer(bufferType))
		{
			desc = D3D12_SHADER_RESOURCE_VIEW_DESC{
				.Format = DXGI_FORMAT_UNKNOWN,
				.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
				.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
				.Buffer = {
					.FirstElement = 0,
					.NumElements = numElements,
					.StructureByteStride = structureByteStride }
			};
		}

		return desc;
	}

	std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> GPUBufferDesc::ToUnorderedAccessViewDesc() const
	{
		check(bufferType != EBufferType::Unknown);

		std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> desc{};
		if (IsUnorderdAccessViewCompatibleBuffer(bufferType) && bIsShaderReadWritable)
		{
			desc = D3D12_UNORDERED_ACCESS_VIEW_DESC{
				.Format = DXGI_FORMAT_UNKNOWN,
				.ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
				.Buffer = {
					.FirstElement = 0,
					.NumElements = numElements,
					.StructureByteStride = structureByteStride,
					.Flags = D3D12_BUFFER_UAV_FLAG_NONE }
			};
		}

		return desc;
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