#include <PCH.h>
#include <D3D12/GpuTextureDesc.h>

namespace ig
{
    size_t SizeOfTexelInBits(const DXGI_FORMAT format)
    {
        switch (static_cast<int>(format))
        {
            case DXGI_FORMAT_R32G32B32A32_TYPELESS:
            case DXGI_FORMAT_R32G32B32A32_FLOAT:
            case DXGI_FORMAT_R32G32B32A32_UINT:
            case DXGI_FORMAT_R32G32B32A32_SINT:
                return 128;

            case DXGI_FORMAT_R32G32B32_TYPELESS:
            case DXGI_FORMAT_R32G32B32_FLOAT:
            case DXGI_FORMAT_R32G32B32_UINT:
            case DXGI_FORMAT_R32G32B32_SINT:
                return 96;

            case DXGI_FORMAT_R16G16B16A16_TYPELESS:
            case DXGI_FORMAT_R16G16B16A16_FLOAT:
            case DXGI_FORMAT_R16G16B16A16_UNORM:
            case DXGI_FORMAT_R16G16B16A16_UINT:
            case DXGI_FORMAT_R16G16B16A16_SNORM:
            case DXGI_FORMAT_R16G16B16A16_SINT:
            case DXGI_FORMAT_R32G32_TYPELESS:
            case DXGI_FORMAT_R32G32_FLOAT:
            case DXGI_FORMAT_R32G32_UINT:
            case DXGI_FORMAT_R32G32_SINT:
            case DXGI_FORMAT_R32G8X24_TYPELESS:
            case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
            case DXGI_FORMAT_Y416:
            case DXGI_FORMAT_Y210:
            case DXGI_FORMAT_Y216:
                return 64;

            case DXGI_FORMAT_R10G10B10A2_TYPELESS:
            case DXGI_FORMAT_R10G10B10A2_UNORM:
            case DXGI_FORMAT_R10G10B10A2_UINT:
            case DXGI_FORMAT_R11G11B10_FLOAT:
            case DXGI_FORMAT_R8G8B8A8_TYPELESS:
            case DXGI_FORMAT_R8G8B8A8_UNORM:
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            case DXGI_FORMAT_R8G8B8A8_UINT:
            case DXGI_FORMAT_R8G8B8A8_SNORM:
            case DXGI_FORMAT_R8G8B8A8_SINT:
            case DXGI_FORMAT_R16G16_TYPELESS:
            case DXGI_FORMAT_R16G16_FLOAT:
            case DXGI_FORMAT_R16G16_UNORM:
            case DXGI_FORMAT_R16G16_UINT:
            case DXGI_FORMAT_R16G16_SNORM:
            case DXGI_FORMAT_R16G16_SINT:
            case DXGI_FORMAT_R32_TYPELESS:
            case DXGI_FORMAT_D32_FLOAT:
            case DXGI_FORMAT_R32_FLOAT:
            case DXGI_FORMAT_R32_UINT:
            case DXGI_FORMAT_R32_SINT:
            case DXGI_FORMAT_R24G8_TYPELESS:
            case DXGI_FORMAT_D24_UNORM_S8_UINT:
            case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
            case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
            case DXGI_FORMAT_R8G8_B8G8_UNORM:
            case DXGI_FORMAT_G8R8_G8B8_UNORM:
            case DXGI_FORMAT_B8G8R8A8_UNORM:
            case DXGI_FORMAT_B8G8R8X8_UNORM:
            case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
            case DXGI_FORMAT_B8G8R8A8_TYPELESS:
            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            case DXGI_FORMAT_B8G8R8X8_TYPELESS:
            case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            case DXGI_FORMAT_AYUV:
            case DXGI_FORMAT_Y410:
            case DXGI_FORMAT_YUY2:
                return 32;

            case DXGI_FORMAT_P010:
            case DXGI_FORMAT_P016:
                return 24;

            case DXGI_FORMAT_R8G8_TYPELESS:
            case DXGI_FORMAT_R8G8_UNORM:
            case DXGI_FORMAT_R8G8_UINT:
            case DXGI_FORMAT_R8G8_SNORM:
            case DXGI_FORMAT_R8G8_SINT:
            case DXGI_FORMAT_R16_TYPELESS:
            case DXGI_FORMAT_R16_FLOAT:
            case DXGI_FORMAT_D16_UNORM:
            case DXGI_FORMAT_R16_UNORM:
            case DXGI_FORMAT_R16_UINT:
            case DXGI_FORMAT_R16_SNORM:
            case DXGI_FORMAT_R16_SINT:
            case DXGI_FORMAT_B5G6R5_UNORM:
            case DXGI_FORMAT_B5G5R5A1_UNORM:
            case DXGI_FORMAT_A8P8:
            case DXGI_FORMAT_B4G4R4A4_UNORM:
                return 16;

            case DXGI_FORMAT_NV12:
            case DXGI_FORMAT_420_OPAQUE:
            case DXGI_FORMAT_NV11:
                return 12;

            case DXGI_FORMAT_R8_TYPELESS:
            case DXGI_FORMAT_R8_UNORM:
            case DXGI_FORMAT_R8_UINT:
            case DXGI_FORMAT_R8_SNORM:
            case DXGI_FORMAT_R8_SINT:
            case DXGI_FORMAT_A8_UNORM:
            case DXGI_FORMAT_AI44:
            case DXGI_FORMAT_IA44:
            case DXGI_FORMAT_P8:
                return 8;

            case DXGI_FORMAT_R1_UNORM:
                return 1;

            case DXGI_FORMAT_BC1_TYPELESS:
            case DXGI_FORMAT_BC1_UNORM:
            case DXGI_FORMAT_BC1_UNORM_SRGB:
            case DXGI_FORMAT_BC4_TYPELESS:
            case DXGI_FORMAT_BC4_UNORM:
            case DXGI_FORMAT_BC4_SNORM:
                return 4;

            case DXGI_FORMAT_BC2_TYPELESS:
            case DXGI_FORMAT_BC2_UNORM:
            case DXGI_FORMAT_BC2_UNORM_SRGB:
            case DXGI_FORMAT_BC3_TYPELESS:
            case DXGI_FORMAT_BC3_UNORM:
            case DXGI_FORMAT_BC3_UNORM_SRGB:
            case DXGI_FORMAT_BC5_TYPELESS:
            case DXGI_FORMAT_BC5_UNORM:
            case DXGI_FORMAT_BC5_SNORM:
            case DXGI_FORMAT_BC6H_TYPELESS:
            case DXGI_FORMAT_BC6H_UF16:
            case DXGI_FORMAT_BC6H_SF16:
            case DXGI_FORMAT_BC7_TYPELESS:
            case DXGI_FORMAT_BC7_UNORM:
            case DXGI_FORMAT_BC7_UNORM_SRGB:
                return 8;

            default:
                return 0;
        }
    }

    uint16_t CalculateMaxNumMipLevels(const uint64_t width, const uint64_t height, const uint64_t depth)
    {
        uint64_t highest = width > height ? width : height;
        highest = highest > depth ? highest : depth;
        return static_cast<uint16_t>(std::log2(highest)) + 1;
    }

    bool IsDepthStencilFormat(const DXGI_FORMAT format)
    {
        return format == DXGI_FORMAT_D16_UNORM || format == DXGI_FORMAT_D24_UNORM_S8_UINT ||
               format == DXGI_FORMAT_D32_FLOAT || format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    }

    bool IsTypelessFormat(const DXGI_FORMAT format)
    {
        return format == DXGI_FORMAT_R32G32B32A32_TYPELESS || format == DXGI_FORMAT_R32G32B32_TYPELESS ||
               format == DXGI_FORMAT_R16G16B16A16_TYPELESS || format == DXGI_FORMAT_R32G32_TYPELESS ||
               format == DXGI_FORMAT_R32G8X24_TYPELESS || format == DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS ||
               format == DXGI_FORMAT_X32_TYPELESS_G8X24_UINT || format == DXGI_FORMAT_R10G10B10A2_TYPELESS ||
               format == DXGI_FORMAT_R8G8B8A8_TYPELESS || format == DXGI_FORMAT_R16G16_TYPELESS ||
               format == DXGI_FORMAT_R32_TYPELESS || format == DXGI_FORMAT_R24G8_TYPELESS ||
               format == DXGI_FORMAT_R24_UNORM_X8_TYPELESS || format == DXGI_FORMAT_X24_TYPELESS_G8_UINT ||
               format == DXGI_FORMAT_R8G8_TYPELESS || format == DXGI_FORMAT_R16_TYPELESS ||
               format == DXGI_FORMAT_R8_TYPELESS || format == DXGI_FORMAT_BC1_TYPELESS ||
               format == DXGI_FORMAT_BC2_TYPELESS || format == DXGI_FORMAT_BC3_TYPELESS ||
               format == DXGI_FORMAT_BC4_TYPELESS || format == DXGI_FORMAT_BC5_TYPELESS ||
               format == DXGI_FORMAT_R8G8B8A8_TYPELESS || format == DXGI_FORMAT_B8G8R8X8_TYPELESS ||
               format == DXGI_FORMAT_BC6H_TYPELESS || format == DXGI_FORMAT_BC7_TYPELESS;
    }

    void GPUTextureDesc::AsTexture1D(const uint32_t width, const uint16_t mipLevels, const DXGI_FORMAT format,
                                     const bool bEnableShaderReadWrite, const bool bEnableSimultaneousAccess)
    {
        bIsArray = false;
        bIsMSAAEnabled = false;
        bIsCubemap = false;
        bIsShaderReadWrite = bEnableShaderReadWrite;
        bIsAllowSimultaneousAccess = bEnableSimultaneousAccess;

        IG_VERIFY(width > 0);
        Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
        Alignment = 0;
        Width = width;
        Height = 1;
        DepthOrArraySize = 1;
        MipLevels = mipLevels;
        Format = format;
        SampleDesc.Count = 1;
        SampleDesc.Quality = 0;
        Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        Flags = bIsShaderReadWrite ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
        Flags = bIsAllowSimultaneousAccess ? Flags | D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS : Flags;
        SamplerFeedbackMipRegion = { .Width = 0, .Height = 0, .Depth = 0 };
    }

    void GPUTextureDesc::AsTexture2D(const uint32_t width, const uint32_t height, const uint16_t mipLevels,
                                     const DXGI_FORMAT format, const bool bEnableShaderReadWrite /*= false*/,
                                     const bool bEnableSimultaneousAccess /*= false*/,
                                     const bool bEnableMSAA /*= false*/, const uint32_t sampleCount /*= 1*/,
                                     uint32_t sampleQuality /*= 0*/)
    {
        IG_VERIFY(!bEnableMSAA || (bEnableMSAA && !bEnableSimultaneousAccess));
        bIsArray = false;
        bIsMSAAEnabled = bEnableMSAA;
        bIsCubemap = false;
        bIsShaderReadWrite = bEnableShaderReadWrite;
        bIsAllowSimultaneousAccess = bEnableSimultaneousAccess;

        IG_VERIFY(width > 0 && height > 0);
        Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        Alignment = 0;
        Width = width;
        Height = height;
        DepthOrArraySize = 1;
        MipLevels = mipLevels;
        Format = format;
        SampleDesc.Count = bIsMSAAEnabled ? sampleCount : 1;
        SampleDesc.Quality = bIsMSAAEnabled ? sampleQuality : 0;
        Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        Flags = bIsShaderReadWrite ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
        Flags = bIsAllowSimultaneousAccess ? Flags | D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS : Flags;
        SamplerFeedbackMipRegion = { .Width = 0, .Height = 0, .Depth = 0 };
    }

    void GPUTextureDesc::AsTexture3D(const uint32_t width, const uint32_t height, const uint16_t depth,
                                     const uint16_t mipLevels, const DXGI_FORMAT format,
                                     const bool bEnableShaderReadWrite,
                                     const bool bEnableSimultaneousAccess /*= false*/,
                                     const bool bEnableMSAA /*= false*/, const uint32_t sampleCount /*= 1*/,
                                     uint32_t sampleQuality /*= 0*/)
    {
        IG_VERIFY(!bEnableMSAA || (bEnableMSAA && !bEnableSimultaneousAccess));
        bIsArray = false;
        bIsMSAAEnabled = bEnableMSAA;
        bIsCubemap = false;
        bIsShaderReadWrite = bEnableShaderReadWrite;
        bIsAllowSimultaneousAccess = bEnableSimultaneousAccess;

        IG_VERIFY(width > 0 && height > 0 && depth > 0);
        Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
        Alignment = 0;
        Width = width;
        Height = height;
        DepthOrArraySize = depth;
        MipLevels = mipLevels;
        Format = format;
        SampleDesc.Count = bIsMSAAEnabled ? sampleCount : 1;
        SampleDesc.Quality = bIsMSAAEnabled ? sampleQuality : 0;
        Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        Flags = bIsShaderReadWrite ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
        Flags = bIsAllowSimultaneousAccess ? Flags | D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS : Flags;
        SamplerFeedbackMipRegion = { .Width = 0, .Height = 0, .Depth = 0 };
    }

    void GPUTextureDesc::AsRenderTarget(const uint32_t width, const uint32_t height, const uint16_t mipLevels,
                                        const DXGI_FORMAT format, const bool bEnableSimultaneousAccess /*= false*/,
                                        const bool bEnableMSAA /*= false*/, const uint32_t sampleCount /*= 1*/,
                                        uint32_t sampleQuality /*= 0*/)
    {
        IG_VERIFY(!bEnableMSAA || (bEnableMSAA && !bEnableSimultaneousAccess));
        bIsArray = false;
        bIsMSAAEnabled = bEnableMSAA;
        bIsCubemap = false;
        bIsShaderReadWrite = false;
        bIsAllowSimultaneousAccess = bEnableSimultaneousAccess;

        IG_VERIFY(width > 0 && height > 0);
        Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        Alignment = 0;
        Width = width;
        Height = height;
        DepthOrArraySize = 1;
        MipLevels = mipLevels;
        Format = format;
        SampleDesc.Count = bIsMSAAEnabled ? sampleCount : 1;
        SampleDesc.Quality = bIsMSAAEnabled ? sampleQuality : 0;
        Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        InitialLayout = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
        Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        Flags = bIsAllowSimultaneousAccess ? Flags | D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS : Flags;
        SamplerFeedbackMipRegion = { .Width = 0, .Height = 0, .Depth = 0 };
    }

    void GPUTextureDesc::AsDepthStencil(const uint32_t width, const uint32_t height, const DXGI_FORMAT format)
    {
        bIsArray = false;
        bIsMSAAEnabled = false;
        bIsCubemap = false;
        bIsShaderReadWrite = false;
        bIsAllowSimultaneousAccess = false;

        IG_VERIFY(width > 0 && height > 0);
        IG_VERIFY(IsDepthStencilFormat(format));
        Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        Alignment = 0;
        Width = width;
        Height = height;
        DepthOrArraySize = 1;
        MipLevels = 1;
        Format = format;
        SampleDesc.Count = 1;
        SampleDesc.Quality = 0;
        Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        InitialLayout = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE;
        Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        SamplerFeedbackMipRegion = { .Width = 0, .Height = 0, .Depth = 0 };
    }

    void GPUTextureDesc::AsTexture1DArray(const uint32_t width, const uint16_t arrayLength, const uint16_t mipLevels,
                                          const DXGI_FORMAT format, const bool bEnableShaderReadWrite /*= false*/,
                                          const bool bEnableSimultaneousAccess /*= false*/)
    {
        bIsArray = true;
        bIsMSAAEnabled = false;
        bIsCubemap = false;
        bIsShaderReadWrite = bEnableShaderReadWrite;
        bIsAllowSimultaneousAccess = bEnableSimultaneousAccess;

        IG_VERIFY(width > 0 && arrayLength > 0);
        Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
        Alignment = 0;
        Width = width;
        Height = 1;
        DepthOrArraySize = arrayLength;
        MipLevels = mipLevels;
        Format = format;
        SampleDesc.Count = 1;
        SampleDesc.Quality = 0;
        Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        Flags = bIsShaderReadWrite ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
        Flags = bIsAllowSimultaneousAccess ? Flags | D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS : Flags;
        SamplerFeedbackMipRegion = { .Width = 0, .Height = 0, .Depth = 0 };
    }

    void GPUTextureDesc::AsTexture2DArray(const uint32_t width, const uint32_t height, const uint16_t arrayLength,
                                          const uint16_t mipLevels, const DXGI_FORMAT format,
                                          const bool bEnableShaderReadWrite /*= false*/,
                                          const bool bEnableSimultaneousAccess /*= false*/,
                                          const bool bEnableMSAA /*= false*/, const uint32_t sampleCount /*= 1*/,
                                          uint32_t sampleQuality /*= 0*/)
    {
        IG_VERIFY(!bEnableMSAA || (bEnableMSAA && !bEnableSimultaneousAccess));
        bIsArray = true;
        bIsMSAAEnabled = bEnableMSAA;
        bIsCubemap = false;
        bIsShaderReadWrite = bEnableShaderReadWrite;
        bIsAllowSimultaneousAccess = bEnableSimultaneousAccess;

        IG_VERIFY(width > 0 && height > 0 && arrayLength > 0);
        Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        Alignment = 0;
        Width = width;
        Height = height;
        DepthOrArraySize = arrayLength;
        MipLevels = mipLevels;
        Format = format;
        SampleDesc.Count = bIsMSAAEnabled ? sampleCount : 1;
        SampleDesc.Quality = bIsMSAAEnabled ? sampleQuality : 0;
        Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        Flags = bIsShaderReadWrite ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
        Flags = bIsAllowSimultaneousAccess ? Flags | D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS : Flags;
        SamplerFeedbackMipRegion = { .Width = 0, .Height = 0, .Depth = 0 };
    }

    void GPUTextureDesc::AsCubemap(const uint32_t width, const uint32_t height, const uint16_t mipLevels,
                                   const DXGI_FORMAT format, const bool bEnableShaderReadWrite /*= false*/,
                                   const bool bEnableSimultaneousAccess /*= false*/, const bool bEnableMSAA /*= false*/,
                                   const uint32_t sampleCount /*= 1*/, uint32_t sampleQuality /*= 0*/)
    {
        IG_VERIFY(!bEnableMSAA || (bEnableMSAA && !bEnableSimultaneousAccess));
        bIsArray = true;
        bIsMSAAEnabled = bEnableMSAA;
        bIsCubemap = true;
        bIsShaderReadWrite = bEnableShaderReadWrite;
        bIsAllowSimultaneousAccess = bEnableSimultaneousAccess;

        IG_VERIFY(width > 0 && height > 0);
        Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        Alignment = 0;
        Width = width;
        Height = height;
        DepthOrArraySize = 6;
        MipLevels = mipLevels;
        Format = format;
        SampleDesc.Count = bIsMSAAEnabled ? sampleCount : 1;
        SampleDesc.Quality = bIsMSAAEnabled ? sampleQuality : 0;
        Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        Flags = bIsShaderReadWrite ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
        Flags = bIsAllowSimultaneousAccess ? Flags | D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS : Flags;
        SamplerFeedbackMipRegion = { .Width = 0, .Height = 0, .Depth = 0 };
    }

    bool GPUTextureDesc::IsUnorderedAccessCompatible() const
    {
        return bIsShaderReadWrite;
    }

    bool GPUTextureDesc::IsDepthStencilCompatible() const
    {
        return IsTexture2D() && IsDepthStencilFormat(Format);
    }

    bool GPUTextureDesc::IsRenderTargetCompatible() const
    {
        return ((Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) == D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) &&
               !IsUnorderedAccessCompatible() &&
               !IsDepthStencilCompatible();
    }

    D3D12MA::ALLOCATION_DESC GPUTextureDesc::GetAllocationDesc() const
    {
        D3D12MA::ALLOCATION_DESC desc{};
        desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        return desc;
    }

    std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> GPUTextureDesc::ConvertToNativeDesc(const GpuTextureSrvDesc& srvDesc, const DXGI_FORMAT desireViewFormat) const
    {
        if (Format == DXGI_FORMAT_UNKNOWN)
        {
            IG_CHECK_NO_ENTRY();
            return std::nullopt;
        }

        if (IsTypelessFormat(Format) && desireViewFormat == DXGI_FORMAT_UNKNOWN)
        {
            IG_CHECK_NO_ENTRY();
            return std::nullopt;
        }

        D3D12_SHADER_RESOURCE_VIEW_DESC newDesc{};
        newDesc.Format = desireViewFormat;
        newDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        /* Configure Shader Resource View Description */
        if (std::holds_alternative<D3D12_TEX1D_SRV>(srvDesc))
        {
            if (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE1D)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            newDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
            newDesc.Texture1D = std::get<D3D12_TEX1D_SRV>(srvDesc);
        }
        else if (std::holds_alternative<D3D12_TEX2D_SRV>(srvDesc))
        {
            if (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            newDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            newDesc.Texture2D = std::get<D3D12_TEX2D_SRV>(srvDesc);
        }
        else if (std::holds_alternative<D3D12_TEX2DMS_SRV>(srvDesc))
        {
            if (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || !bIsMSAAEnabled)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            newDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
            newDesc.Texture2DMS = std::get<D3D12_TEX2DMS_SRV>(srvDesc);
        }
        else if (std::holds_alternative<D3D12_TEX3D_SRV>(srvDesc))
        {
            if (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            newDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        }
        else if (std::holds_alternative<D3D12_TEXCUBE_SRV>(srvDesc))
        {
            if (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || !bIsCubemap)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            if (DepthOrArraySize < 6)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            newDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
            newDesc.TextureCube = std::get<D3D12_TEXCUBE_SRV>(srvDesc);
        }
        else if (std::holds_alternative<D3D12_TEX1D_ARRAY_SRV>(srvDesc))
        {
            if (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE1D)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            if (DepthOrArraySize <= 1)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            newDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
            newDesc.Texture1DArray = std::get<D3D12_TEX1D_ARRAY_SRV>(srvDesc);
        }
        else if (std::holds_alternative<D3D12_TEX2D_ARRAY_SRV>(srvDesc))
        {
            if (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            if (DepthOrArraySize <= 1)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            newDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            newDesc.Texture2DArray = std::get<D3D12_TEX2D_ARRAY_SRV>(srvDesc);
        }
        else if (std::holds_alternative<D3D12_TEX2DMS_ARRAY_SRV>(srvDesc))
        {
            if (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || !bIsMSAAEnabled)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            if (DepthOrArraySize <= 1)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            newDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
            newDesc.Texture2DMSArray = std::get<D3D12_TEX2DMS_ARRAY_SRV>(srvDesc);
        }
        else if (std::holds_alternative<D3D12_TEXCUBE_ARRAY_SRV>(srvDesc))
        {
            if (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || !bIsCubemap)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            if (DepthOrArraySize < 6)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            newDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
            newDesc.TextureCubeArray = std::get<D3D12_TEXCUBE_ARRAY_SRV>(srvDesc);
        }
        else
        {
            IG_CHECK_NO_ENTRY();
            return std::nullopt;
        }

        return std::make_optional(newDesc);
    }

    std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> GPUTextureDesc::ConvertToNativeDesc(const GpuTextureUavDesc& uavDesc, const DXGI_FORMAT desireViewFormat) const
    {
        if (!IsUnorderedAccessCompatible())
        {
            IG_CHECK_NO_ENTRY();
            return std::nullopt;
        }

        if (Format == DXGI_FORMAT_UNKNOWN)
        {
            IG_CHECK_NO_ENTRY();
            return std::nullopt;
        }

        if (IsTypelessFormat(Format) && desireViewFormat == DXGI_FORMAT_UNKNOWN)
        {
            IG_CHECK_NO_ENTRY();
            return std::nullopt;
        }

        D3D12_UNORDERED_ACCESS_VIEW_DESC newDesc{};
        newDesc.Format = desireViewFormat;

        if (std::holds_alternative<D3D12_TEX1D_UAV>(uavDesc))
        {
            if (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE1D)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            newDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
            newDesc.Texture1D = std::get<D3D12_TEX1D_UAV>(uavDesc);
        }
        else if (std::holds_alternative<D3D12_TEX2D_UAV>(uavDesc))
        {
            if (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            newDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            newDesc.Texture2D = std::get<D3D12_TEX2D_UAV>(uavDesc);
        }
        else if (std::holds_alternative<D3D12_TEX2DMS_UAV>(uavDesc))
        {
            if (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || !bIsMSAAEnabled)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            newDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMS;
            newDesc.Texture2DMS = std::get<D3D12_TEX2DMS_UAV>(uavDesc);
        }
        else if (std::holds_alternative<D3D12_TEX3D_UAV>(uavDesc))
        {
            if (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            newDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
            newDesc.Texture3D = std::get<D3D12_TEX3D_UAV>(uavDesc);
        }
        else if (std::holds_alternative<D3D12_TEX1D_ARRAY_UAV>(uavDesc))
        {
            if (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE1D)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            if (DepthOrArraySize <= 1)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            newDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
            newDesc.Texture1DArray = std::get<D3D12_TEX1D_ARRAY_UAV>(uavDesc);
        }
        else if (std::holds_alternative<D3D12_TEX2D_ARRAY_UAV>(uavDesc))
        {
            if (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            if (DepthOrArraySize <= 1)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            newDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            newDesc.Texture2DArray = std::get<D3D12_TEX2D_ARRAY_UAV>(uavDesc);
        }
        else if (std::holds_alternative<D3D12_TEX2DMS_ARRAY_UAV>(uavDesc))
        {
            if (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || !bIsMSAAEnabled)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            if (DepthOrArraySize <= 1)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            newDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY;
            newDesc.Texture2DMSArray = std::get<D3D12_TEX2DMS_ARRAY_UAV>(uavDesc);
        }
        else
        {
            IG_CHECK_NO_ENTRY();
            return std::nullopt;
        }

        return std::make_optional(newDesc);
    }

    std::optional<D3D12_RENDER_TARGET_VIEW_DESC> GPUTextureDesc::ConvertToNativeDesc(const GpuTextureRtvDesc& rtvDesc, const DXGI_FORMAT desireViewFormat) const
    {
        if (!IsRenderTargetCompatible())
        {
            IG_CHECK_NO_ENTRY();
            return std::nullopt;
        }

        if (Format == DXGI_FORMAT_UNKNOWN)
        {
            IG_CHECK_NO_ENTRY();
            return std::nullopt;
        }

        if (IsTypelessFormat(Format) && desireViewFormat == DXGI_FORMAT_UNKNOWN)
        {
            IG_CHECK_NO_ENTRY();
            return std::nullopt;
        }

        D3D12_RENDER_TARGET_VIEW_DESC newDesc{};
        newDesc.Format = desireViewFormat;

        if (std::holds_alternative<D3D12_TEX1D_RTV>(rtvDesc))
        {
            if (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE1D)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            newDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
            newDesc.Texture1D = std::get<D3D12_TEX1D_RTV>(rtvDesc);
        }
        else if (std::holds_alternative<D3D12_TEX2D_RTV>(rtvDesc))
        {
            if (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            newDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            newDesc.Texture2D = std::get<D3D12_TEX2D_RTV>(rtvDesc);
        }
        else if (std::holds_alternative<D3D12_TEX2DMS_RTV>(rtvDesc))
        {
            if (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE1D || !bIsMSAAEnabled)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            newDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
            newDesc.Texture2DMS = std::get<D3D12_TEX2DMS_RTV>(rtvDesc);
        }
        else if (std::holds_alternative<D3D12_TEX3D_RTV>(rtvDesc))
        {
            if (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            newDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
            newDesc.Texture3D = std::get<D3D12_TEX3D_RTV>(rtvDesc);
        }
        else if (std::holds_alternative<D3D12_TEX1D_ARRAY_RTV>(rtvDesc))
        {
            if (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE1D)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            if (DepthOrArraySize <= 1)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            newDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
            newDesc.Texture1DArray = std::get<D3D12_TEX1D_ARRAY_RTV>(rtvDesc);
        }
        else if (std::holds_alternative<D3D12_TEX2D_ARRAY_RTV>(rtvDesc))
        {
            if (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            if (DepthOrArraySize <= 1)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            newDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            newDesc.Texture2DArray = std::get<D3D12_TEX2D_ARRAY_RTV>(rtvDesc);
        }
        else if (std::holds_alternative<D3D12_TEX2DMS_ARRAY_RTV>(rtvDesc))
        {
            if (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || !bIsMSAAEnabled)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            if (DepthOrArraySize <= 1)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            newDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
            newDesc.Texture2DMSArray = std::get<D3D12_TEX2DMS_ARRAY_RTV>(rtvDesc);
        }
        else
        {
            IG_CHECK_NO_ENTRY();
            return std::nullopt;
        }

        return std::make_optional(newDesc);
    }

    std::optional<D3D12_DEPTH_STENCIL_VIEW_DESC> GPUTextureDesc::ConvertToNativeDesc(const GpuTextureDsvDesc& dsvDesc, const DXGI_FORMAT desireViewFormat) const
    {
        if (!IsDepthStencilCompatible())
        {
            IG_CHECK_NO_ENTRY();
            return std::nullopt;
        }

        if (desireViewFormat != DXGI_FORMAT_UNKNOWN && !IsDepthStencilFormat(desireViewFormat))
        {
            IG_CHECK_NO_ENTRY();
            return std::nullopt;
        }

        D3D12_DEPTH_STENCIL_VIEW_DESC newDesc{};
        newDesc.Format = desireViewFormat;

        if (std::holds_alternative<D3D12_TEX1D_DSV>(dsvDesc))
        {
            if (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE1D)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            newDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
            newDesc.Texture1D = std::get<D3D12_TEX1D_DSV>(dsvDesc);
        }
        else if (std::holds_alternative<D3D12_TEX2D_DSV>(dsvDesc))
        {
            if (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            newDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            newDesc.Texture2D = std::get<D3D12_TEX2D_DSV>(dsvDesc);
        }
        else if (std::holds_alternative<D3D12_TEX2DMS_DSV>(dsvDesc))
        {
            if (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || !bIsMSAAEnabled)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            newDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
            newDesc.Texture2DMS = std::get<D3D12_TEX2DMS_DSV>(dsvDesc);
        }
        else if (std::holds_alternative<D3D12_TEX1D_ARRAY_DSV>(dsvDesc))
        {
            if (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE1D)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            if (DepthOrArraySize <= 1)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            newDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
            newDesc.Texture1DArray = std::get<D3D12_TEX1D_ARRAY_DSV>(dsvDesc);
        }
        else if (std::holds_alternative<D3D12_TEX2D_ARRAY_DSV>(dsvDesc))
        {
            if (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            if (DepthOrArraySize <= 1)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            newDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            newDesc.Texture2DArray = std::get<D3D12_TEX2D_ARRAY_DSV>(dsvDesc);
        }
        else if (std::holds_alternative<D3D12_TEX2DMS_ARRAY_DSV>(dsvDesc))
        {
            if (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE1D)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            if (DepthOrArraySize <= 1)
            {
                IG_CHECK_NO_ENTRY();
                return std::nullopt;
            }

            newDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
            newDesc.Texture2DMSArray = std::get<D3D12_TEX2DMS_ARRAY_DSV>(dsvDesc);
        }
        else
        {
            IG_CHECK_NO_ENTRY();
            return std::nullopt;
        }

        return std::make_optional(newDesc);
    }

    void GPUTextureDesc::From(const D3D12_RESOURCE_DESC& desc)
    {
        IG_VERIFY(desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER && desc.Dimension != D3D12_RESOURCE_DIMENSION_UNKNOWN);
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