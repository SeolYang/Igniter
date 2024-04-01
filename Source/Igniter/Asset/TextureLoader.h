#pragma once
#include <Igniter.h>
#include <Core/Result.h>
#include <Asset/Texture.h>

namespace ig
{
    enum class ETextureLoaderStatus
    {
        Success,
        InvalidAssetInfo,
        AssetTypeMismatch,
        InvalidDimensions,
        UnknownFormat,
        FileDoesNotExists,
        FailedLoadFromFile,
        DimensionsMismatch,
        CubemapFlagMismatch,
        InvalidCubemapArrayLength,
        DimensionFlagMismatch,
        FormatMismatch,
        FailedCreateTexture,
        FailedCreateShaderResourceView,
        FailedCreateSamplerView,
    };

    class HandleManager;
    class RenderDevice;
    class GpuUploader;
    class GpuViewManager;
    class TextureLoader final
    {
    public:
        TextureLoader(HandleManager& handleManager, RenderDevice& renderDevice, GpuUploader& gpuUploader, GpuViewManager& gpuViewManager);
        TextureLoader(const TextureLoader&) = delete;
        TextureLoader(TextureLoader&&) noexcept = delete;
        ~TextureLoader() = default;

        TextureLoader& operator=(const TextureLoader&) = delete;
        TextureLoader& operator=(TextureLoader&&) noexcept = delete;

        Result<Texture, ETextureLoaderStatus> Load(const Texture::Desc& desc);

    private:
        HandleManager& handleManager;
        RenderDevice& renderDevice;
        GpuUploader& gpuUploader;
        GpuViewManager& gpuViewManager;
    };
}