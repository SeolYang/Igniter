#pragma once
#include <Core/String.h>
#include <D3D12/Commons.h>
#include <span>
#include <optional>

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
	struct GPUBufferDescription
	{
	public:
		static GPUBufferDescription BuildConstantBufferDescription(const uint32_t sizeOfBufferInBytes);
		static GPUBufferDescription BuildStructuredBufferDescription(const uint32_t sizeOfElementInBytes, const uint32_t numOfElements, const bool bIsDynamic = false);
		static GPUBufferDescription BuildUploadBufferDescription(const uint32_t sizeOfBufferInBytes);
		static GPUBufferDescription BuildReadbackBufferDescription(const uint32_t sizeOfBufferInBytes);
		static GPUBufferDescription BuildVertexBufferDescription(const uint32_t sizeOfVertexInBytes, const uint32_t numVertices);
		static GPUBufferDescription BuildIndexBufferDescription(const uint32_t sizeOfIndexInBytes, const uint32_t numIndices);

		bool IsConstantBufferCompatible() const;
		bool IsShaderResourceCompatible() const;
		bool IsUnorderedAccessCompatible() const;
		bool IsUploadCompatible() const;
		bool IsReadbackCompatible() const;

		D3D12_RESOURCE_DESC1							AsResourceDescription() const;
		D3D12MA::ALLOCATION_DESC						AsAllocationDescription() const;
		std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC>	AsShaderResourceViewDescription() const;
		std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC>	AsConstantBufferViewDescription(const D3D12_GPU_VIRTUAL_ADDRESS bufferLocation) const;
		std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> AsUnorderedAccessViewDescription() const;

	public:
		fe::String			 DebugName = fe::String{ "Buffer" };
		uint32_t			 SizeInBytes = 1;
		uint32_t			 NumElements = 1;
		uint32_t			 ElementStrideInBytes = 1;
		bool				 bIsDynamic = false;
		bool				 bIsCpuAccessible = false;
		D3D12_BUFFER_BARRIER StandardBarrier = {};
	};
} // namespace fe
