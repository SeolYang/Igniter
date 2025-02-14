#include "Igniter/Igniter.h"
#include "Igniter/D3D12/GpuBufferDesc.h"

namespace ig
{
    void GpuBufferDesc::AsConstantBuffer(const U32 sizeOfBufferInBytes)
    {
        IG_VERIFY(sizeOfBufferInBytes > 0);
        bIsShaderReadWritable = false;
        bIsCpuAccessible = true;
        bufferType = EGpuBufferType::ConstantBuffer;

        structureByteStride = sizeOfBufferInBytes;
        numElements = 1;

        Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        Alignment = 0;
        Width = AdjustSizeForConstantBuffer(sizeOfBufferInBytes);
        Height = 1;
        DepthOrArraySize = 1;
        MipLevels = 1;
        SampleDesc = {.Count = 1, .Quality = 0};
        Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Flags = D3D12_RESOURCE_FLAG_NONE;
        Format = DXGI_FORMAT_UNKNOWN;
    }

    void GpuBufferDesc::AsStructuredBuffer(const U32 sizeOfElementInBytes, const U32 numOfElements, const bool bShouldEnableShaderReadWrite /*= false*/, const bool bShouldEnableUavCounter /*= false*/)
    {
        IG_VERIFY(sizeOfElementInBytes > 0);
        IG_VERIFY(numOfElements > 0);
        bIsShaderReadWritable = bShouldEnableShaderReadWrite;
        bIsUavCounterEnabled = bShouldEnableUavCounter;
        bIsCpuAccessible = false;
        bufferType = EGpuBufferType::StructuredBuffer;

        structureByteStride = sizeOfElementInBytes;
        numElements = numOfElements;

        Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        Alignment = 0;
        Width = (Size)sizeOfElementInBytes * numOfElements;
        if (bShouldEnableUavCounter)
        {
            Width = kUavCounterSize + (((U32)Width + (D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT - 1));
        }

        Height = 1;
        DepthOrArraySize = 1;
        MipLevels = 1;
        SampleDesc = {.Count = 1, .Quality = 0};
        Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Flags = bIsShaderReadWritable ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
        Format = DXGI_FORMAT_UNKNOWN;
    }

    void GpuBufferDesc::AsUploadBuffer(const U32 sizeOfBufferInBytes, const bool bIsShaderResource)
    {
        IG_VERIFY(sizeOfBufferInBytes > 0);
        bIsShaderReadWritable = false;
        bIsCpuAccessible = true;
        bufferType = EGpuBufferType::UploadBuffer;

        structureByteStride = sizeOfBufferInBytes;
        numElements = 1;

        Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        Alignment = 0;
        Width = sizeOfBufferInBytes;
        Height = 1;
        DepthOrArraySize = 1;
        MipLevels = 1;
        SampleDesc = {.Count = 1, .Quality = 0};
        Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Flags = bIsShaderResource ? D3D12_RESOURCE_FLAG_NONE : D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        Format = DXGI_FORMAT_UNKNOWN;
    }

    void GpuBufferDesc::AsReadbackBuffer(const U32 sizeOfBufferInBytes)
    {
        IG_VERIFY(sizeOfBufferInBytes > 0);
        bIsShaderReadWritable = false;
        bIsCpuAccessible = true;
        bufferType = EGpuBufferType::ReadbackBuffer;

        structureByteStride = sizeOfBufferInBytes;
        numElements = 1;

        Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        Alignment = 0;
        Width = sizeOfBufferInBytes;
        Height = 1;
        DepthOrArraySize = 1;
        MipLevels = 1;
        SampleDesc = {.Count = 1, .Quality = 0};
        Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Flags = D3D12_RESOURCE_FLAG_NONE;
        Format = DXGI_FORMAT_UNKNOWN;
    }

    void GpuBufferDesc::AsVertexBuffer(const U32 sizeOfVertexInBytes, const U32 numVertices)
    {
        IG_VERIFY(sizeOfVertexInBytes > 0);
        IG_VERIFY(numVertices > 0);
        bIsShaderReadWritable = false;
        bIsCpuAccessible = false;
        bufferType = EGpuBufferType::VertexBuffer;

        structureByteStride = sizeOfVertexInBytes;
        numElements = numVertices;

        Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        Alignment = 0;
        Width = static_cast<Size>(sizeOfVertexInBytes) * numVertices;
        Height = 1;
        DepthOrArraySize = 1;
        MipLevels = 1;
        SampleDesc = {.Count = 1, .Quality = 0};
        Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Flags = D3D12_RESOURCE_FLAG_NONE;
    }

    void GpuBufferDesc::AsIndexBuffer(const U32 sizeOfIndexInBytes, const U32 numIndices)
    {
        IG_VERIFY(sizeOfIndexInBytes > 0);
        IG_VERIFY(numIndices > 0);
        bIsShaderReadWritable = false;
        bIsCpuAccessible = false;
        bufferType = EGpuBufferType::IndexBuffer;

        structureByteStride = sizeOfIndexInBytes;
        numElements = numIndices;

        Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        Alignment = 0;
        Width = static_cast<Size>(sizeOfIndexInBytes) * numIndices;
        Height = 1;
        DepthOrArraySize = 1;
        MipLevels = 1;
        SampleDesc = {.Count = 1, .Quality = 0};
        Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Flags = D3D12_RESOURCE_FLAG_NONE;
    }

    D3D12MA::ALLOCATION_DESC GpuBufferDesc::GetAllocationDesc() const
    {
        IG_CHECK(bufferType != EGpuBufferType::Unknown);
        D3D12MA::ALLOCATION_DESC desc{.HeapType = D3D12_HEAP_TYPE_DEFAULT};
        if (HeapType != kAutoHeapType)
        {
            desc.HeapType = HeapType;
        }
        else
        {
            switch (bufferType)
            {
            case EGpuBufferType::ConstantBuffer:
                desc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
                break;
            case EGpuBufferType::UploadBuffer:
                desc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
                break;
            case EGpuBufferType::GpuUploadBuffer:
                desc.HeapType = D3D12_HEAP_TYPE_GPU_UPLOAD;
                break;
            case EGpuBufferType::ReadbackBuffer:
                desc.HeapType = D3D12_HEAP_TYPE_READBACK;
                break;
            default:
                break;
            }
        }

        return desc;
    }

    std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC> GpuBufferDesc::ToConstantBufferViewDesc(const D3D12_GPU_VIRTUAL_ADDRESS bufferLocation) const
    {
        IG_CHECK(bufferType != EGpuBufferType::Unknown);

        std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC> desc{};
        if (IsConstantBufferViewCompatible(bufferType))
        {
            desc = D3D12_CONSTANT_BUFFER_VIEW_DESC{.BufferLocation = bufferLocation, .SizeInBytes = static_cast<U32>(Width)};
        }

        return desc;
    }

    std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> GpuBufferDesc::ToShaderResourceViewDesc() const
    {
        IG_CHECK(bufferType != EGpuBufferType::Unknown);

        std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> desc{};
        if (IsShaderResourceViewCompatible(bufferType))
        {
            desc = D3D12_SHADER_RESOURCE_VIEW_DESC{
                .Format = bIsRawBuffer  ? DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_UNKNOWN,
                .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
                .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                .Buffer = {
                    .FirstElement = 0,
                    .NumElements = numElements,
                    .StructureByteStride = bIsRawBuffer ? 0Ui32 : structureByteStride,
                    .Flags = bIsRawBuffer ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE
                }
            };
        }

        return desc;
    }

    std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> GpuBufferDesc::ToUnorderedAccessViewDesc() const
    {
        IG_CHECK(bufferType != EGpuBufferType::Unknown);

        std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> desc{};
        if (IsUnorderedAccessViewCompatible(bufferType) && bIsShaderReadWritable)
        {
            desc = D3D12_UNORDERED_ACCESS_VIEW_DESC{
                .Format = DXGI_FORMAT_UNKNOWN,
                .ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
                .Buffer =
                {
                    .FirstElement = 0,
                    .NumElements = numElements,
                    .StructureByteStride = structureByteStride,
                    .CounterOffsetInBytes = GetUavCounterOffset(),
                    .Flags = D3D12_BUFFER_UAV_FLAG_NONE
                }
            };
        }

        return desc;
    }

    void GpuBufferDesc::From(const D3D12_RESOURCE_DESC& desc)
    {
        IG_VERIFY(desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER);
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
} // namespace ig
