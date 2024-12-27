#pragma once
#include "Igniter/Core/String.h"
#include "Igniter/D3D12/Common.h"

namespace ig
{
    inline uint32_t AdjustSizeForConstantBuffer(const uint32_t originalSizeInBytes)
    {
        return ((originalSizeInBytes / D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) + 1) * D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
    }

    enum class EGpuBufferType : U8
    {
        ConstantBuffer,
        StructuredBuffer,
        UploadBuffer,
        ReadbackBuffer,
        VertexBuffer,
        IndexBuffer,
        Unknown
    };

    constexpr bool IsCpuAccessible(const EGpuBufferType type)
    {
        return type == EGpuBufferType::ConstantBuffer || type == EGpuBufferType::UploadBuffer || type == EGpuBufferType::ReadbackBuffer;
    }

    constexpr bool IsConstantBufferViewCompatible(const EGpuBufferType type)
    {
        return type == EGpuBufferType::ConstantBuffer;
    }

    constexpr bool IsShaderResourceViewCompatible(const EGpuBufferType type)
    {
        return type == EGpuBufferType::StructuredBuffer;
    }

    constexpr bool IsUnorderedAccessViewCompatible(const EGpuBufferType type)
    {
        return type == EGpuBufferType::StructuredBuffer;
    }

    constexpr bool IsUploadCompatible(const EGpuBufferType type)
    {
        return type == EGpuBufferType::UploadBuffer;
    }

    constexpr bool IsReadbackCompatible(const EGpuBufferType type)
    {
        return type == EGpuBufferType::ReadbackBuffer;
    }

    constexpr bool IsIndexBufferCompatible(const EGpuBufferType type)
    {
        return type == EGpuBufferType::IndexBuffer || type == EGpuBufferType::StructuredBuffer;
    }

    /* #sy_todo 기본 값 설정 */
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

        [[nodiscard]] DXGI_FORMAT GetIndexFormat() const noexcept
        {
            if (!IsIndexBufferCompatible(bufferType))
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

        [[nodiscard]] bool           IsCpuAccessible() const noexcept { return bIsCpuAccessible; }
        [[nodiscard]] EGpuBufferType GetBufferType() const noexcept { return bufferType; }
        [[nodiscard]] uint32_t       GetStructureByteStride() const noexcept { return structureByteStride; }
        [[nodiscard]] uint32_t       GetNumElements() const noexcept { return numElements; }
        [[nodiscard]] uint64_t       GetSizeAsBytes() const noexcept { return Width; }

        [[nodiscard]] D3D12MA::ALLOCATION_DESC                        GetAllocationDesc() const;
        [[nodiscard]] std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC>  ToConstantBufferViewDesc(const D3D12_GPU_VIRTUAL_ADDRESS bufferLocation) const;
        [[nodiscard]] std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC>  ToShaderResourceViewDesc() const;
        [[nodiscard]] std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> ToUnorderedAccessViewDesc() const;

        void From(const D3D12_RESOURCE_DESC& desc);

    private:
        void AsVertexBuffer(const uint32_t sizeOfVertexInBytes, const uint32_t numVertices);
        void AsIndexBuffer(const uint32_t sizeOfIndexInBytes, const uint32_t numIndices);

    public:
        String DebugName = String{"Unknown Buffer"};

    private:
        uint32_t       structureByteStride   = 1;
        uint32_t       numElements           = 1;
        EGpuBufferType bufferType            = EGpuBufferType::Unknown;
        bool           bIsShaderReadWritable = false;
        bool           bIsCpuAccessible      = false;
        D3D12MA::Pool* customPool            = nullptr;
    };
} // namespace ig
