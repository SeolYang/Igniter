#pragma once
#include <Igniter.h>
#include <Core/Result.h>
#include <Asset/Texture.h>

namespace ig::details
{
    enum class EMakeDefaultTexStatus
    {
        Success,
        InvalidAssetInfo,
        FailedCreateTexture,
        FailedCreateShaderResourceView,
        FailedCreateSamplerView,
    };
}

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
    class RenderContext;
    class AssetManager;

    class TextureLoader final
    {
        friend class AssetManager;

    public:
        TextureLoader(HandleManager& handleManager, RenderDevice& renderDevice, RenderContext& renderContext);
        TextureLoader(const TextureLoader&)     = delete;
        TextureLoader(TextureLoader&&) noexcept = delete;
        ~TextureLoader()                        = default;

        TextureLoader& operator=(const TextureLoader&)     = delete;
        TextureLoader& operator=(TextureLoader&&) noexcept = delete;

    private:
        Result<Texture, ETextureLoaderStatus>           Load(const Texture::Desc& desc);
        Result<Texture, details::EMakeDefaultTexStatus> MakeDefault(const AssetInfo& assetInfo);
        Result<Texture, details::EMakeDefaultTexStatus> MakeMonochrome(const AssetInfo& assetInfo, const Color& color);

    private:
        HandleManager& handleManager;
        RenderDevice&  renderDevice;
        RenderContext& renderContext;
    };
} // namespace ig
