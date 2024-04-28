#pragma once
#include <Core/Handle.h>
#include <Core/String.h>
#include <D3D12/Common.h>
#include <Render/RenderContext.h>
#include <Asset/Common.h>

namespace ig
{
    class Texture;
    template <>
    constexpr inline EAssetCategory AssetCategoryOf<Texture> = EAssetCategory::Texture;

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
        Json& Serialize(Json& archive) const;
        const Json& Deserialize(const Json& archive);

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
        Json& Serialize(Json& archive) const;
        const Json& Deserialize(const Json& archive);

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

    class GpuTexture;
    class GpuView;
    class RenderContext;
    class Texture final
    {
    public:
        using ImportDesc = TextureImportDesc;
        using LoadDesc = TextureLoadDesc;
        using Desc = AssetDesc<Texture>;

    public:
        Texture(RenderContext& renderContext, const Desc& snapshot, const RenderResource<GpuTexture> gpuTexture, const RenderResource<GpuView> srv, const RenderResource<GpuView> sampler);
        Texture(const Texture&) = delete;
        Texture(Texture&&) noexcept = default;
        ~Texture();

        Texture& operator=(const Texture&) = delete;
        Texture& operator=(Texture&&) noexcept = default;

        const Desc& GetSnapshot() const { return snapshot; }
        RenderResource<GpuTexture> GetGpuTexture() const { return gpuTexture; }
        RenderResource<GpuView> GetShaderResourceView() const { return srv; }
        RenderResource<GpuView> GetSampler() const { return sampler; }

    public:
        /* #sy_wip Common으로 이동 */
        static constexpr std::string_view EngineDefault = "Engine\\Default";
        static constexpr std::string_view EngineDefaultWhite = "Engine\\White";
        static constexpr std::string_view EngineDefaultBlack = "Engine\\Black";

    private:
        RenderContext* renderContext{nullptr};
        Desc snapshot{};
        RenderResource<GpuTexture> gpuTexture{};
        RenderResource<GpuView> srv{};
        RenderResource<GpuView> sampler{};
    };
}    // namespace ig

namespace ig::details
{
    ETextureDimension AsTexDimension(const DirectX::TEX_DIMENSION dim);
}
