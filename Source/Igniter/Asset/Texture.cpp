#include <Igniter.h>
#include <Core/JsonUtils.h>
#include <Core/Serializable.h>
#include <D3D12/GpuTexture.h>
#include <Render/GpuViewManager.h>
#include <Asset/Texture.h>

namespace ig
{
    json& TextureImportDesc::Serialize(json& archive) const
    {
        const TextureImportDesc& config = *this;
        IG_SERIALIZE_ENUM_JSON(TextureImportDesc, archive, config, CompressionMode);
        IG_SERIALIZE_JSON(TextureImportDesc, archive, config, bGenerateMips);
        IG_SERIALIZE_ENUM_JSON(TextureImportDesc, archive, config, Filter);
        IG_SERIALIZE_ENUM_JSON(TextureImportDesc, archive, config, AddressModeU);
        IG_SERIALIZE_ENUM_JSON(TextureImportDesc, archive, config, AddressModeV);
        IG_SERIALIZE_ENUM_JSON(TextureImportDesc, archive, config, AddressModeW);
        return archive;
    }

    const json& TextureImportDesc::Deserialize(const json& archive)
    {
        *this = {};
        TextureImportDesc& config = *this;
        IG_DESERIALIZE_ENUM_JSON(TextureImportDesc, archive, config, CompressionMode, ETextureCompressionMode::None);
        IG_DESERIALIZE_JSON(TextureImportDesc, archive, config, bGenerateMips, false);
        IG_DESERIALIZE_ENUM_JSON(TextureImportDesc, archive, config, Filter, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
        IG_DESERIALIZE_ENUM_JSON(TextureImportDesc, archive, config, AddressModeU, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        IG_DESERIALIZE_ENUM_JSON(TextureImportDesc, archive, config, AddressModeV, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        IG_DESERIALIZE_ENUM_JSON(TextureImportDesc, archive, config, AddressModeW, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        return archive;
    }

    json& TextureLoadDesc::Serialize(json& archive) const
    {
        const TextureLoadDesc& config = *this;
        IG_CHECK(config.Format != DXGI_FORMAT_UNKNOWN);
        IG_CHECK(config.Width > 0 && config.Height > 0 && config.DepthOrArrayLength > 0 && config.Mips > 0);
        IG_SERIALIZE_ENUM_JSON(TextureLoadDesc, archive, config, Format);
        IG_SERIALIZE_ENUM_JSON(TextureLoadDesc, archive, config, Dimension);
        IG_SERIALIZE_JSON(TextureLoadDesc, archive, config, Width);
        IG_SERIALIZE_JSON(TextureLoadDesc, archive, config, Height);
        IG_SERIALIZE_JSON(TextureLoadDesc, archive, config, DepthOrArrayLength);
        IG_SERIALIZE_JSON(TextureLoadDesc, archive, config, Mips);
        IG_SERIALIZE_JSON(TextureLoadDesc, archive, config, bIsCubemap);
        IG_SERIALIZE_ENUM_JSON(TextureLoadDesc, archive, config, Filter);
        IG_SERIALIZE_ENUM_JSON(TextureLoadDesc, archive, config, AddressModeU);
        IG_SERIALIZE_ENUM_JSON(TextureLoadDesc, archive, config, AddressModeV);
        IG_SERIALIZE_ENUM_JSON(TextureLoadDesc, archive, config, AddressModeW);
        return archive;
    }

    const json& TextureLoadDesc::Deserialize(const json& archive)
    {
        TextureLoadDesc& config = *this;
        config = {};
        IG_DESERIALIZE_ENUM_JSON(TextureLoadDesc, archive, config, Format, DXGI_FORMAT_UNKNOWN);
        IG_DESERIALIZE_ENUM_JSON(TextureLoadDesc, archive, config, Dimension, ETextureDimension::Tex2D);
        IG_DESERIALIZE_JSON(TextureLoadDesc, archive, config, Width, 0);
        IG_DESERIALIZE_JSON(TextureLoadDesc, archive, config, Height, 0);
        IG_DESERIALIZE_JSON(TextureLoadDesc, archive, config, DepthOrArrayLength, 0);
        IG_DESERIALIZE_JSON(TextureLoadDesc, archive, config, Mips, 0);
        IG_DESERIALIZE_JSON(TextureLoadDesc, archive, config, bIsCubemap, false);
        IG_DESERIALIZE_ENUM_JSON(TextureLoadDesc, archive, config, Filter, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
        IG_DESERIALIZE_ENUM_JSON(TextureLoadDesc, archive, config, AddressModeU, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        IG_DESERIALIZE_ENUM_JSON(TextureLoadDesc, archive, config, AddressModeV, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        IG_DESERIALIZE_ENUM_JSON(TextureLoadDesc, archive, config, AddressModeW, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        return archive;
    }

    Texture::Texture(Desc snapshot, DeferredHandle<GpuTexture> gpuTexture, Handle<GpuView, GpuViewManager*> srv, RefHandle<GpuView> sampler)
        : snapshot(snapshot),
          gpuTexture(std::move(gpuTexture)),
          srv(std::move(srv)),
          sampler(sampler)
    {
    }

    Texture::~Texture()
    {
    }
} // namespace ig

namespace ig::details
{
    ETextureDimension AsTexDimension(const DirectX::TEX_DIMENSION dim)
    {
        switch (dim)
        {
            case DirectX::TEX_DIMENSION_TEXTURE1D:
                return ETextureDimension::Tex1D;
            default:
            case DirectX::TEX_DIMENSION_TEXTURE2D:
                return ETextureDimension::Tex2D;
            case DirectX::TEX_DIMENSION_TEXTURE3D:
                return ETextureDimension::Tex3D;
        }
    }
} // namespace ig::details