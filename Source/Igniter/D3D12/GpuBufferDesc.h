#pragma once
#include "Igniter/Core/String.h"
#include "Igniter/D3D12/Common.h"

namespace ig
{
    inline U32 AdjustSizeForConstantBuffer(const U32 originalSizeInBytes)
    {
        return ((originalSizeInBytes / D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) + 1) * D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
    }

    enum class EGpuBufferType : U8
    {
        ConstantBuffer,
        StructuredBuffer,
        UploadBuffer,
        GpuUploadBuffer,
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
        switch (type)
        {
        case EGpuBufferType::ConstantBuffer:
        case EGpuBufferType::StructuredBuffer:
        case EGpuBufferType::UploadBuffer:
        case EGpuBufferType::GpuUploadBuffer:
            return true;
        default:
            return false;
        }
    }

    constexpr bool IsShaderResourceViewCompatible(const EGpuBufferType type)
    {
        switch (type)
        {
        case EGpuBufferType::ConstantBuffer:
        case EGpuBufferType::StructuredBuffer:
        case EGpuBufferType::UploadBuffer:
        case EGpuBufferType::GpuUploadBuffer:
            return true;
        default:
            return false;
        }
    }

    constexpr bool IsUnorderedAccessViewCompatible(const EGpuBufferType type)
    {
        switch (type)
        {
        case EGpuBufferType::ConstantBuffer:
        case EGpuBufferType::StructuredBuffer:
        case EGpuBufferType::GpuUploadBuffer:
            return true;
        default:
            return false;
        }
    }

    constexpr bool IsUploadCompatible(const EGpuBufferType type)
    {
        switch (type)
        {
        case EGpuBufferType::ConstantBuffer:
        case EGpuBufferType::UploadBuffer:
        case EGpuBufferType::GpuUploadBuffer:
            return true;
        default:
            return false;
        }
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
        void AsConstantBuffer(const U32 sizeOfBufferInBytes);
        void AsStructuredBuffer(const U32 sizeOfElementInBytes, const U32 numOfElements, const bool bShouldEnableShaderReadWrite = false, const bool bShouldEnableUavCounter = false);
        void AsUploadBuffer(const U32 sizeOfBufferInBytes, const bool bIsShaderResource = false);
        void AsGpuUploadBuffer(const U32 sizeOfBufferInBytes, const bool bShouldEnableShaderReadWrite = false)
        {
            AsUploadBuffer(sizeOfBufferInBytes);
            Flags = bShouldEnableShaderReadWrite ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
        }
        void AsReadbackBuffer(const U32 sizeOfBufferInBytes);

        template <typename T>
        void AsConstantBuffer()
        {
            AsConstantBuffer(sizeof(T));
        }

        template <typename T>
        void AsStructuredBuffer(const U32 numOfElements, const bool bShouldEnableShaderReadWrite = false, const bool bShouldEnableUavCounter = false)
        {
            AsStructuredBuffer(sizeof(T), numOfElements, bShouldEnableShaderReadWrite, bShouldEnableUavCounter);
        }

        template <typename T>
        void AsVertexBuffer(const U32 numVertices)
        {
            AsVertexBuffer(sizeof(T), numVertices);
        }

        template <typename T>
            requires std::same_as<T, uint8_t> || std::same_as<T, uint16_t> || std::same_as<T, U32>
        void AsIndexBuffer(const U32 numIndices)
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

        [[nodiscard]] bool IsCpuAccessible() const noexcept { return bIsCpuAccessible; }
        [[nodiscard]] bool IsUavCounterEnabled() const noexcept { return bIsUavCounterEnabled; }
        [[nodiscard]] EGpuBufferType GetBufferType() const noexcept { return bufferType; }
        [[nodiscard]] U32 GetStructureByteStride() const noexcept { return structureByteStride; }
        [[nodiscard]] U32 GetNumElements() const noexcept { return numElements; }
        [[nodiscard]] uint64_t GetSizeAsBytes() const noexcept { return Width; }
        [[nodiscard]] Size GetUavCounterOffset() const noexcept
        {
            return bIsUavCounterEnabled ? (Width - kUavCounterSize) : 0Ui64;
        }

        [[nodiscard]] D3D12MA::ALLOCATION_DESC GetAllocationDesc() const;
        [[nodiscard]] std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC> ToConstantBufferViewDesc(const D3D12_GPU_VIRTUAL_ADDRESS bufferLocation) const;
        [[nodiscard]] std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> ToShaderResourceViewDesc() const;
        [[nodiscard]] std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> ToUnorderedAccessViewDesc() const;

        void From(const D3D12_RESOURCE_DESC& desc);

      private:
        void AsVertexBuffer(const U32 sizeOfVertexInBytes, const U32 numVertices);
        void AsIndexBuffer(const U32 sizeOfIndexInBytes, const U32 numIndices);

      public:
        using UavCounter = U32;
        constexpr static Size kUavCounterSize = sizeof(UavCounter);
        String DebugName = String{"Unknown Buffer"};
        D3D12_HEAP_TYPE HeapType = kAutoHeapType;
        bool bIsRawBuffer = false;

      private:
        constexpr static D3D12_HEAP_TYPE kAutoHeapType = D3D12_HEAP_TYPE(0xFFFFFFFF);
        U32 structureByteStride = 1;
        U32 numElements = 1;
        EGpuBufferType bufferType = EGpuBufferType::Unknown;
        bool bIsShaderReadWritable = false;
        bool bIsCpuAccessible = false;
        bool bIsUavCounterEnabled = false;
        D3D12MA::Pool* customPool = nullptr;
    };
} // namespace ig
