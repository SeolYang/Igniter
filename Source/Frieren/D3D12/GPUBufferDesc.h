#pragma once
#include <span>
#include <optional>

#include <Core/String.h>
#include <D3D12/Common.h>

namespace fe::dx
{
	inline uint32_t AdjustSizeForConstantBuffer(const uint32_t originalSizeInBytes)
	{
		constexpr uint32_t MinimumConstantBufferSizeInBytes = 256;
		return ((originalSizeInBytes / MinimumConstantBufferSizeInBytes) + 1) * MinimumConstantBufferSizeInBytes;
	}

	class GPUBufferDesc : public D3D12_RESOURCE_DESC1
	{
	public:
		void AsConstantBuffer(const uint32_t sizeOfBufferInBytes);
		void AsStructuredBuffer(const uint32_t sizeOfElementInBytes, const uint32_t numOfElements, const bool bEnableShaderReadWrtie = false);
		void AsUploadBuffer(const uint32_t sizeOfBufferInBytes);
		void AsReadbackBuffer(const uint32_t sizeOfBufferInBytes);
		void AsVertexBuffer(const uint32_t sizeOfVertexInBytes, const uint32_t numVertices);
		void AsIndexBuffer(const uint32_t sizeOfIndexInBytes, const uint32_t numIndices);

		template <typename T>
		void AsConstantBuffer()
		{
			AsConstantBuffer(sizeof(T));
		}

		template <typename T>
		void AsStructuredBuffer(const uint32_t numOfElements, const bool bEnableShaderReadWrtie = false)
		{
			AsStructuredBuffer(sizeof(T), numOfElements, bEnableShaderReadWrtie);
		}

		template <typename T>
		void AsVertexBuffer(const uint32_t numVertices)
		{
			AsVertexBuffer(sizeof(T), numVertices);
		}

		template <typename T>
		void AsIndexBuffer(const uint32_t numIndices)
		{
			AsIndexBuffer(sizeof(T), numIndices);
		}

		bool IsConstantBufferCompatible() const;
		bool IsShaderResourceCompatible() const;
		bool IsUnorderedAccessCompatible() const;
		bool IsUploadCompatible() const;
		bool IsReadbackCompatible() const;
		bool IsCPUAccessible() const { return bIsCPUAccessible; }
		D3D12_BUFFER_BARRIER GetStandardBarrier() const { return standardBarrier; }

		uint32_t GetStructureByteStride() const { return structureByteStride; }
		uint32_t GetNumElements() const { return numElements; }

		D3D12MA::ALLOCATION_DESC						ToAllocationDesc() const;
		std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC>	ToShaderResourceViewDesc() const;
		std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC>	ToConstantBufferViewDesc(const D3D12_GPU_VIRTUAL_ADDRESS bufferLocation) const;
		std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> ToUnorderedAccessViewDesc() const;

		void From(const D3D12_RESOURCE_DESC& desc);

	public:
		String DebugName = String{ "Unknown Buffer" };

	private:
		bool				 bIsShaderReadWritable = false;
		bool				 bIsCPUAccessible = false;
		D3D12_BUFFER_BARRIER standardBarrier = {};

		uint32_t structureByteStride = 1;
		uint32_t numElements = 1;
	};
} // namespace fe
