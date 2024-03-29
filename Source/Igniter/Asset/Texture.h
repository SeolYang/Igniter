#pragma once
#include <Core/Handle.h>
#include <Core/String.h>
#include <Core/Result.h>
#include <D3D12/Common.h>
#include <D3D12/GpuTexture.h>
#include <Asset/Common.h>

struct ID3D11Device;

namespace ig
{
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

    struct TextureImportConfig
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

    struct TextureLoadConfig
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
    struct Texture
    {
    public:
        using ImportConfigType = TextureImportConfig;
        using LoadConfigType = TextureLoadConfig;
        using MetadataType = std::pair<AssetInfo, LoadConfigType>;

    public:
        xg::Guid Guid {};
        TextureLoadConfig LoadConfig{};
        Handle<GpuTexture, DeferredDestroyer<GpuTexture>> TextureInstance{}; // Using Deferred Deallocation
        Handle<GpuView, GpuViewManager*> ShaderResourceView{};
        RefHandle<GpuView> TexSampler{};
    };

    template <>
    constexpr inline EAssetType AssetTypeOf_v<Texture> = EAssetType::Texture;

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

        Result<Texture::MetadataType, ETextureImportStatus> Import(const String resPathStr, TextureImportConfig config);

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

namespace ig::details
{
} // namespace ig::details