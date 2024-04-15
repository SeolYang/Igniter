#include <Igniter.h>
#include <Core/Json.h>
#include <D3D12/GpuTexture.h>
#include <Render/GpuViewManager.h>
#include <Asset/Texture.h>

namespace ig
{
    json& TextureImportDesc::Serialize(json& archive) const
    {
        IG_SERIALIZE_ENUM_JSON_SIMPLE(TextureImportDesc, archive, CompressionMode);
        IG_SERIALIZE_JSON_SIMPLE(TextureImportDesc, archive, bGenerateMips);
        IG_SERIALIZE_ENUM_JSON_SIMPLE(TextureImportDesc, archive, Filter);
        IG_SERIALIZE_ENUM_JSON_SIMPLE(TextureImportDesc, archive, AddressModeU);
        IG_SERIALIZE_ENUM_JSON_SIMPLE(TextureImportDesc, archive, AddressModeV);
        IG_SERIALIZE_ENUM_JSON_SIMPLE(TextureImportDesc, archive, AddressModeW);
        return archive;
    }

    const json& TextureImportDesc::Deserialize(const json& archive)
    {
        *this = {};
        IG_DESERIALIZE_ENUM_JSON_SIMPLE(TextureImportDesc, archive, CompressionMode, ETextureCompressionMode::None);
        IG_DESERIALIZE_JSON_SIMPLE(TextureImportDesc, archive, bGenerateMips, false);
        IG_DESERIALIZE_ENUM_JSON_SIMPLE(TextureImportDesc, archive, Filter, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
        IG_DESERIALIZE_ENUM_JSON_SIMPLE(TextureImportDesc, archive, AddressModeU, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        IG_DESERIALIZE_ENUM_JSON_SIMPLE(TextureImportDesc, archive, AddressModeV, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        IG_DESERIALIZE_ENUM_JSON_SIMPLE(TextureImportDesc, archive, AddressModeW, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        return archive;
    }

    json& TextureLoadDesc::Serialize(json& archive) const
    {
        IG_SERIALIZE_ENUM_JSON_SIMPLE(TextureLoadDesc, archive, Format);
        IG_SERIALIZE_ENUM_JSON_SIMPLE(TextureLoadDesc, archive, Dimension);
        IG_SERIALIZE_JSON_SIMPLE(TextureLoadDesc, archive, Width);
        IG_SERIALIZE_JSON_SIMPLE(TextureLoadDesc, archive, Height);
        IG_SERIALIZE_JSON_SIMPLE(TextureLoadDesc, archive, DepthOrArrayLength);
        IG_SERIALIZE_JSON_SIMPLE(TextureLoadDesc, archive, Mips);
        IG_SERIALIZE_JSON_SIMPLE(TextureLoadDesc, archive, bIsCubemap);
        IG_SERIALIZE_ENUM_JSON_SIMPLE(TextureLoadDesc, archive, Filter);
        IG_SERIALIZE_ENUM_JSON_SIMPLE(TextureLoadDesc, archive, AddressModeU);
        IG_SERIALIZE_ENUM_JSON_SIMPLE(TextureLoadDesc, archive, AddressModeV);
        IG_SERIALIZE_ENUM_JSON_SIMPLE(TextureLoadDesc, archive, AddressModeW);
        return archive;
    }

    const json& TextureLoadDesc::Deserialize(const json& archive)
    {
        *this = {};
        IG_DESERIALIZE_ENUM_JSON_SIMPLE(TextureLoadDesc, archive, Format, DXGI_FORMAT_UNKNOWN);
        IG_DESERIALIZE_ENUM_JSON_SIMPLE(TextureLoadDesc, archive, Dimension, ETextureDimension::Tex2D);
        IG_DESERIALIZE_JSON_SIMPLE(TextureLoadDesc, archive, Width, 0);
        IG_DESERIALIZE_JSON_SIMPLE(TextureLoadDesc, archive, Height, 0);
        IG_DESERIALIZE_JSON_SIMPLE(TextureLoadDesc, archive, DepthOrArrayLength, 0);
        IG_DESERIALIZE_JSON_SIMPLE(TextureLoadDesc, archive, Mips, 0);
        IG_DESERIALIZE_JSON_SIMPLE(TextureLoadDesc, archive, bIsCubemap, false);
        IG_DESERIALIZE_ENUM_JSON_SIMPLE(TextureLoadDesc, archive, Filter, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
        IG_DESERIALIZE_ENUM_JSON_SIMPLE(TextureLoadDesc, archive, AddressModeU, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        IG_DESERIALIZE_ENUM_JSON_SIMPLE(TextureLoadDesc, archive, AddressModeV, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        IG_DESERIALIZE_ENUM_JSON_SIMPLE(TextureLoadDesc, archive, AddressModeW, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        return archive;
    }

    Texture::Texture(const Desc& snapshot, DeferredHandle<GpuTexture> gpuTexture, Handle<GpuView, GpuViewManager*> srv,
                     const RefHandle<GpuView>& sampler)
        : snapshot(snapshot)
        , gpuTexture(std::move(gpuTexture))
        , srv(std::move(srv))
        , sampler(sampler)
    {
        IG_CHECK(this->gpuTexture);
        IG_CHECK(this->srv);
        IG_CHECK(this->sampler);
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
