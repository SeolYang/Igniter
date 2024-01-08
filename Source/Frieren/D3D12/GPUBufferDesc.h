#pragma once
#include <span>
#include <optional>

#include <Core/String.h>
#include <D3D12/Common.h>

namespace fe::Private
{
	inline uint32_t AdjustSizeForConstantBuffer(const uint32_t originalSizeInBytes)
	{
		constexpr uint32_t MinimumConstantBufferSizeInBytes = 256;
		return ((originalSizeInBytes / MinimumConstantBufferSizeInBytes) + 1) * MinimumConstantBufferSizeInBytes;
	}
} // namespace fe::Private

namespace fe
{
	class GPUBufferDesc
	{
	public:
		static GPUBufferDesc BuildConstantBuffer(const uint32_t sizeOfBufferInBytes);
		static GPUBufferDesc BuildStructuredBuffer(const uint32_t sizeOfElementInBytes, const uint32_t numOfElements, const bool bIsDynamic = false);
		static GPUBufferDesc BuildUploadBuffer(const uint32_t sizeOfBufferInBytes);
		static GPUBufferDesc BuildReadbackBuffer(const uint32_t sizeOfBufferInBytes);
		static GPUBufferDesc BuildVertexBuffer(const uint32_t sizeOfVertexInBytes, const uint32_t numVertices);
		static GPUBufferDesc BuildIndexBuffer(const uint32_t sizeOfIndexInBytes, const uint32_t numIndices);

		bool IsConstantBufferCompatible() const;
		bool IsShaderResourceCompatible() const;
		bool IsUnorderedAccessCompatible() const;
		bool IsUploadCompatible() const;
		bool IsReadbackCompatible() const;

		D3D12_RESOURCE_DESC1							ToResourceDesc() const;
		D3D12MA::ALLOCATION_DESC						ToAllocationDesc() const;
		std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC>	ToShaderResourceViewDesc() const;
		std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC>	ToConstantBufferViewDesc(const D3D12_GPU_VIRTUAL_ADDRESS bufferLocation) const;
		std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> ToUnorderedAccessViewDesc() const;

	public:
		String				 DebugName = String { "Buffer" };
		uint32_t			 SizeInBytes = 1;
		uint32_t			 NumElements = 1;
		uint32_t			 ElementStrideInBytes = 1;
		bool				 bIsShaderReadWritable = false;
		bool				 bIsCPUAccessible = false;
		D3D12_BUFFER_BARRIER StandardBarrier = {};
	};
} // namespace fe
