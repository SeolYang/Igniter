#pragma once
#include <Core/Container.h>
#include <Core/String.h>
#include <D3D12/Common.h>

namespace fe::dx
{
	inline uint32_t AdjustSizeForConstantBuffer(const uint32_t originalSizeInBytes)
	{
		constexpr uint32_t MinimumConstantBufferSizeInBytes = 256;
		return ((originalSizeInBytes / MinimumConstantBufferSizeInBytes) + 1) * MinimumConstantBufferSizeInBytes;
	}

	enum class EBufferType
	{
		ConstantBuffer,
		StructuredBuffer,
		UploadBuffer,
		ReadbackBuffer,
		VertexBuffer,
		IndexBuffer,
		Unknown
	};

	constexpr bool IsCPUAccessCompatibleBuffer(const EBufferType type)
	{
		return type == EBufferType::ConstantBuffer || type == EBufferType::UploadBuffer || type == EBufferType::ReadbackBuffer;
	}

	constexpr bool IsConstantBufferViewCompatibleBuffer(const EBufferType type)
	{
		return type == EBufferType::ConstantBuffer;
	}

	constexpr bool IsShaderResourceViewCompatibleBuffer(const EBufferType type)
	{
		return type == EBufferType::StructuredBuffer;
	}

	constexpr bool IsUnorderdAccessViewCompatibleBuffer(const EBufferType type)
	{
		return type == EBufferType::StructuredBuffer;
	}

	constexpr bool IsUploadCompatibleBuffer(const EBufferType type)
	{
		return type == EBufferType::UploadBuffer;
	}

	constexpr bool IsReadbackCompatibleBuffer(const EBufferType type)
	{
		return type == EBufferType::ReadbackBuffer;
	}

	class GPUBufferDesc : public D3D12_RESOURCE_DESC1
	{
	public:
		void AsConstantBuffer(const uint32_t sizeOfBufferInBytes);
		void AsStructuredBuffer(const uint32_t sizeOfElementInBytes, const uint32_t numOfElements,
								const bool bEnableShaderReadWrtie = false);
		void AsUploadBuffer(const uint32_t sizeOfBufferInBytes);
		void AsReadbackBuffer(const uint32_t sizeOfBufferInBytes);

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
			requires std::same_as<T, uint8_t> || std::same_as<T, uint16_t> || std::same_as<T, uint32_t>
		void AsIndexBuffer(const uint32_t numIndices)
		{
			AsIndexBuffer(sizeof(T), numIndices);
		}

		DXGI_FORMAT GetIndexFormat() const
		{
			if (bufferType != EBufferType::IndexBuffer)
			{
				return DXGI_FORMAT_UNKNOWN;
			}

			switch (structureByteStride)
			{
				default:
					return DXGI_FORMAT_UNKNOWN;
				case 1:
					return DXGI_FORMAT_R8_UINT;
				case 2:
					return DXGI_FORMAT_R16_UINT;
				case 4:
					return DXGI_FORMAT_R32_UINT;
			}
		}

		bool		IsCPUAccessible() const { return bIsCPUAccessible; }
		EBufferType GetBufferType() const { return bufferType; }
		uint32_t	GetStructureByteStride() const { return structureByteStride; }
		uint32_t	GetNumElements() const { return numElements; }
		uint64_t	GetSizeAsBytes() const { return Width; }

		D3D12MA::ALLOCATION_DESC						ToAllocationDesc() const;
		std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC>	ToConstantBufferViewDesc(const D3D12_GPU_VIRTUAL_ADDRESS bufferLocation) const;
		std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC>	ToShaderResourceViewDesc() const;
		std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> ToUnorderedAccessViewDesc() const;

		void From(const D3D12_RESOURCE_DESC& desc);

	private:
		void AsVertexBuffer(const uint32_t sizeOfVertexInBytes, const uint32_t numVertices);
		void AsIndexBuffer(const uint32_t sizeOfIndexInBytes, const uint32_t numIndices);

	public:
		String DebugName = String{ "Unknown Buffer" };

	private:
		uint32_t	structureByteStride = 1;
		uint32_t	numElements = 1;
		EBufferType bufferType = EBufferType::Unknown;
		bool		bIsShaderReadWritable = false;
		bool		bIsCPUAccessible = false;
	};
} // namespace fe::dx
