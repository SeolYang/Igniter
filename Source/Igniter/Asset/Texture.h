#pragma once
#include <Core/Handle.h>
#include <Core/String.h>
#include <Core/Result.h>
#include <D3D12/Common.h>
#include <Asset/Common.h>

struct ID3D11Device;

namespace ig
{
    struct Texture;
    template <>
    constexpr inline EAssetType AssetTypeOf_v<Texture> = EAssetType::Texture;

    enum class ETextureCompressionMode
    {
        None,
        BC4,  /* Gray-scale */
        BC5,  /* Tangent-space normal maps */
        BC6H, /* HDR images */
        BC7   /* High-quality color maps/Color maps + Full Alpha */
    };

    enum class ETextureDimension
    {
        Tex1D,
        Tex2D,
        Tex3D
    };

    struct TextureImportDesc
    {
    public:
        json& Serialize(json& archive) const;
        const json& Deserialize(const json& archive);

    public:
        ETextureCompressionMode CompressionMode = ETextureCompressionMode::None;
        bool bGenerateMips = false;

        D3D12_FILTER Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        D3D12_TEXTURE_ADDRESS_MODE AddressModeU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        D3D12_TEXTURE_ADDRESS_MODE AddressModeV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        D3D12_TEXTURE_ADDRESS_MODE AddressModeW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    };

    struct TextureLoadDesc
    {
    public:
        json& Serialize(json& archive) const;
        const json& Deserialize(const json& archive);

        [[nodiscard]] bool IsArray() const { return Dimension != ETextureDimension::Tex3D && DepthOrArrayLength > 1; }

    public:
        DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
        ETextureDimension Dimension = ETextureDimension::Tex2D;
        uint32_t Width = 1;
        uint32_t Height = 1;
        uint16_t DepthOrArrayLength = 1;
        uint16_t Mips = 1;
        bool bIsCubemap = false;

        D3D12_FILTER Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        D3D12_TEXTURE_ADDRESS_MODE AddressModeU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        D3D12_TEXTURE_ADDRESS_MODE AddressModeV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        D3D12_TEXTURE_ADDRESS_MODE AddressModeW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    };

    class RenderDevice;
    class GpuTexture;
    class GpuView;
    class GpuViewManager;
    struct Texture final
    {
    public:
        using ImportDesc = TextureImportDesc;
        using LoadDesc = TextureLoadDesc;
        using Desc = AssetDesc<Texture>;

    public:
        Texture(Desc snapshot, DeferredHandle<GpuTexture> gpuTexture, Handle<GpuView, GpuViewManager*> srv, RefHandle<GpuView> sampler);
        Texture(const Texture&) = delete;
        Texture(Texture&&) noexcept = default;
        ~Texture();

        Texture& operator=(const Texture&) = delete;
        Texture& operator=(Texture&&) noexcept = default;

        Desc GetSnapshot() const { return snapshot; }
        RefHandle<GpuTexture> GetGpuTexture() { return gpuTexture.MakeRef(); }
        RefHandle<GpuView> GetShaderResourceView() { return srv.MakeRef(); }
        RefHandle<GpuView> GetSampler() { return sampler; }

    private:
        Desc snapshot{};
        DeferredHandle<GpuTexture> gpuTexture{};
        Handle<GpuView, GpuViewManager*> srv{};
        RefHandle<GpuView> sampler{};
    };

    enum class ETextureImportStatus
    {
        Success,
        FileDoesNotExist,
        UnsupportedExtension,
        FailedLoadFromFile,
        InvalidDimensions,
        InvalidVolumemap,
        UnknownFormat,
        FailedGenerateMips,
        FailedCompression,
        FailedSaveMetadataToFile,
        FailedSaveAssetToFile
    };

    class TextureImporter
    {
    public:
        TextureImporter();
        ~TextureImporter();

        Result<Texture::Desc, ETextureImportStatus> Import(const String resPathStr, TextureImportDesc config);

    private:
        ID3D11Device* d3d11Device = nullptr;
    };

    class GpuUploader;
    class TextureLoader
    {
    public:
        static std::optional<Texture> Load(const xg::Guid& guid, HandleManager& handleManager, RenderDevice& renderDevice, GpuUploader& gpuUploader, GpuViewManager& gpuViewManager);
    };
} // namespace ig
