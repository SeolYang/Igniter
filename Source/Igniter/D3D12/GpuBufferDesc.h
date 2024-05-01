#pragma once
#include "Igniter/Core/String.h"
#include "Igniter/D3D12/Common.h"

namespace ig
{
    inline uint32_t AdjustSizeForConstantBuffer(const uint32_t originalSizeInBytes)
    {
        return ((originalSizeInBytes / D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) + 1) * D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
    }

    enum class EGpuBufferType
    {
        ConstantBuffer,
        StructuredBuffer,
        UploadBuffer,
        ReadbackBuffer,
        VertexBuffer,
        IndexBuffer,
        Unknown
    };

    constexpr bool IsCPUAccessCompatibleBuffer(const EGpuBufferType type)
    {
        return type == EGpuBufferType::ConstantBuffer || type == EGpuBufferType::UploadBuffer || type == EGpuBufferType::ReadbackBuffer;
    }

    constexpr bool IsConstantBufferViewCompatibleBuffer(const EGpuBufferType type)
    {
        return type == EGpuBufferType::ConstantBuffer;
    }

    constexpr bool IsShaderResourceViewCompatibleBuffer(const EGpuBufferType type)
    {
        return type == EGpuBufferType::StructuredBuffer;
    }

    constexpr bool IsUnorderdAccessViewCompatibleBuffer(const EGpuBufferType type)
    {
        return type == EGpuBufferType::StructuredBuffer;
    }

    constexpr bool IsUploadCompatibleBuffer(const EGpuBufferType type)
    {
        return type == EGpuBufferType::UploadBuffer;
    }

    constexpr bool IsReadbackCompatibleBuffer(const EGpuBufferType type)
    {
        return type == EGpuBufferType::ReadbackBuffer;
    }

    class GpuBufferDesc final : public D3D12_RESOURCE_DESC1
    {
    public:
        void AsConstantBuffer(const uint32_t sizeOfBufferInBytes);
        void AsStructuredBuffer(const uint32_t sizeOfElementInBytes, const uint32_t numOfElements, const bool bEnableShaderReadWrtie = false);
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
            if (bufferType != EGpuBufferType::IndexBuffer)
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

        bool IsCPUAccessible() const { return bIsCPUAccessible; }
        EGpuBufferType GetBufferType() const { return bufferType; }
        uint32_t GetStructureByteStride() const { return structureByteStride; }
        uint32_t GetNumElements() const { return numElements; }
        uint64_t GetSizeAsBytes() const { return Width; }

        D3D12MA::ALLOCATION_DESC GetAllocationDesc() const;
        std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC> ToConstantBufferViewDesc(const D3D12_GPU_VIRTUAL_ADDRESS bufferLocation) const;
        std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> ToShaderResourceViewDesc() const;
        std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> ToUnorderedAccessViewDesc() const;

        void From(const D3D12_RESOURCE_DESC& desc);

    private:
        void AsVertexBuffer(const uint32_t sizeOfVertexInBytes, const uint32_t numVertices);
        void AsIndexBuffer(const uint32_t sizeOfIndexInBytes, const uint32_t numIndices);

    public:
        String DebugName = String{"Unknown Buffer"};

    private:
        uint32_t structureByteStride = 1;
        uint32_t numElements = 1;
        EGpuBufferType bufferType = EGpuBufferType::Unknown;
        bool bIsShaderReadWritable = false;
        bool bIsCPUAccessible = false;
        D3D12MA::Pool* customPool = nullptr;
    };
}    // namespace ig
