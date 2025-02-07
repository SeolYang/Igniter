#pragma once
#include "Igniter/Core/Handle.h"
#include "Igniter/Core/String.h"
#include "Igniter/D3D12/Common.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Asset/Common.h"

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
        U32 Width = 1;
        U32 Height = 1;
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
        Texture(RenderContext& renderContext, const Desc& snapshot, const Handle<GpuTexture> gpuTexture, const Handle<GpuView> srv,
                const Handle<GpuView> sampler);
        Texture(const Texture&) = delete;
        Texture(Texture&&) noexcept = default;
        ~Texture();

        Texture& operator=(const Texture&) = delete;
        Texture& operator=(Texture&& rhs) noexcept;

        [[nodiscard]] const Desc& GetSnapshot() const { return snapshot; }
        [[nodiscard]] Handle<GpuTexture> GetGpuTexture() const { return gpuTexture; }
        [[nodiscard]] Handle<GpuView> GetShaderResourceView() const { return srv; }
        [[nodiscard]] Handle<GpuView> GetSampler() const { return sampler; }

      private:
        void Destroy();

      public:
        /* #sy_wip Common으로 이동 */
        static constexpr std::string_view EngineDefault = "Engine\\Default";
        static constexpr std::string_view EngineDefaultWhite = "Engine\\White";
        static constexpr std::string_view EngineDefaultBlack = "Engine\\Black";

      private:
        RenderContext* renderContext{nullptr};
        Desc snapshot{};
        Handle<GpuTexture> gpuTexture{};
        Handle<GpuView> srv{};
        Handle<GpuView> sampler{};
    };
} // namespace ig

namespace ig::details
{
    ETextureDimension AsTexDimension(const DirectX::TEX_DIMENSION dim);
}
