#pragma once
#include "Igniter/Core/String.h"
#include "Igniter/D3D12/Common.h"

namespace ig
{
    Size SizeOfTexelInBits(const DXGI_FORMAT format);
    uint16_t CalculateMaxNumMipLevels(const uint64_t width, const uint64_t height, const uint64_t depth);
    bool IsDepthStencilFormat(const DXGI_FORMAT format);
    bool IsTypelessFormat(const DXGI_FORMAT format);

    class GpuTextureDesc final : public D3D12_RESOURCE_DESC1
    {
    public:
        /* #sy_todo 기본 값 설정 */
        GpuTextureDesc() = default;
        virtual ~GpuTextureDesc() = default;

        void AsTexture1D(const U32 width, const uint16_t mipLevels, const DXGI_FORMAT format, const bool bEnableShaderReadWrite = false,
                         const bool bEnableSimultaneousAccess = false);
        void AsTexture2D(const U32 width, const U32 height, const uint16_t mipLevels, const DXGI_FORMAT format,
                         const bool bEnableShaderReadWrite = false, const bool bEnableSimultaneousAccess = false, const bool bEnableMSAA = false,
                         const U32 sampleCount = 1, U32 sampleQuality = 0);
        void AsTexture3D(const U32 width, const U32 height, const uint16_t depth, const uint16_t mipLevels, const DXGI_FORMAT format,
                         const bool bEnableShaderReadWrite = false, const bool bEnableSimultaneousAccess = false, const bool bEnableMSAA = false,
                         const U32 sampleCount = 1, U32 sampleQuality = 0);

        void AsRenderTarget(const U32 width, const U32 height, const uint16_t mipLevels, const DXGI_FORMAT format,
                            const bool bEnableShaderReadWrite = false,
                            const bool bEnableSimultaneousAccess = false, const bool bEnableMSAA = false, const U32 sampleCount = 1, U32 sampleQuality = 0);
        void AsDepthStencil(const U32 width, const U32 height, const DXGI_FORMAT format, const bool bReverseZ);

        void AsTexture1DArray(const U32 width, const uint16_t arrayLength, const uint16_t mipLevels, const DXGI_FORMAT format,
                              const bool bEnableShaderReadWrite = false, const bool bEnableSimultaneousAccess = false);
        void AsTexture2DArray(const U32 width, const U32 height, const uint16_t arrayLength, const uint16_t mipLevels,
                              const DXGI_FORMAT format, const bool bEnableShaderReadWrite = false, const bool bEnableSimultaneousAccess = false,
                              const bool bEnableMSAA = false, const U32 sampleCount = 1, U32 sampleQuality = 0);

        void AsCubemap(const U32 width, const U32 height, const uint16_t mipLevels, const DXGI_FORMAT format,
                       const bool bEnableShaderReadWrite = false, const bool bEnableSimultaneousAccess = false, const bool bEnableMSAA = false,
                       const U32 sampleCount = 1, U32 sampleQuality = 0);

        bool IsUnorderedAccessCompatible() const;
        bool IsDepthStencilCompatible() const;
        bool IsRenderTargetCompatible() const;

        [[nodiscard]] bool IsTexture1D() const noexcept { return Width > 0 && Height == 1 && DepthOrArraySize == 1; }
        [[nodiscard]] bool IsTexture2D() const noexcept { return Width > 0 && Height > 0 && DepthOrArraySize == 1; }
        [[nodiscard]] bool IsTexture3D() const noexcept { return Width > 0 && Height > 0 && DepthOrArraySize > 0; }
        [[nodiscard]] bool IsTextureArray() const noexcept { return bIsArray; }
        [[nodiscard]] bool IsTexture1DArray() const noexcept { return IsTextureArray() && Width > 0 && Height == 1 && DepthOrArraySize > 0; }
        [[nodiscard]] bool IsTexture2DArray() const noexcept { return IsTextureArray() && Width > 0 && Height > 0 && DepthOrArraySize > 0; }
        [[nodiscard]] bool IsTextureCube() const noexcept { return bIsCubemap; }

        [[nodiscard]] bool IsMSAAEnabled() const noexcept { return bIsMSAAEnabled; }
        [[nodiscard]] bool IsAllowSimultaneousAccess() const noexcept { return bIsAllowSimultaneousAccess; }

        [[nodiscard]] D3D12MA::ALLOCATION_DESC GetAllocationDesc() const;
        [[nodiscard]] std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> ConvertToNativeDesc(
            const GpuTextureSrvVariant& srvVariant, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN) const;
        [[nodiscard]] std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> ConvertToNativeDesc(
            const GpuTextureUavVariant& uavVariant, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN) const;
        [[nodiscard]] std::optional<D3D12_RENDER_TARGET_VIEW_DESC> ConvertToNativeDesc(
            const GpuTextureRtvVariant& rtvVariant, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN) const;
        [[nodiscard]] std::optional<D3D12_DEPTH_STENCIL_VIEW_DESC> ConvertToNativeDesc(
            const GpuTextureDsvVariant& dsvVariant, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN) const;

        void From(const D3D12_RESOURCE_DESC& desc);

        /* #sy_todo https://learn.microsoft.com/en-us/windows/win32/direct3d12/d3d12calcsubresource 에 따르면 3D 텍스처의 Array Size는 항상 0*/
        Size GetNumSubresources() const { return static_cast<Size>(DepthOrArraySize) * MipLevels; }

        U32 GetArraySize() const { return IsTexture3D() ? 0 : DepthOrArraySize; }

        /* #sy_note 간단한 format(비압축/RGBA)과 간단한 구조(Array, Slice)에 대한 subresource 정보만 생성 */
        Vector<D3D12_SUBRESOURCE_DATA> GenerateSubresourcesData(const std::span<uint8_t> memoryBlock) const;

    public:
        std::string_view DebugName = "Unknown Texture";
        D3D12_BARRIER_LAYOUT InitialLayout = D3D12_BARRIER_LAYOUT_COMMON;

        F32 ClearDepthValue = 1.f;
        U8 ClearStencilValue = 0;
        Color ClearColorValue = Color{0.f, 0.f, 0.f, 1.f};

    private:
        bool bIsArray = false;
        bool bIsMSAAEnabled = false;
        bool bIsCubemap = false;
        bool bIsShaderReadWrite = false;
        bool bIsAllowSimultaneousAccess = false;
    };
} // namespace ig
