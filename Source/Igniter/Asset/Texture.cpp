#include "Igniter/Igniter.h"
#include "Igniter/Core/Json.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Asset/Texture.h"

namespace ig
{
    Json& TextureImportDesc::Serialize(Json& archive) const
    {
        IG_SERIALIZE_ENUM_JSON_SIMPLE(TextureImportDesc, archive, CompressionMode);
        IG_SERIALIZE_JSON_SIMPLE(TextureImportDesc, archive, bGenerateMips);
        IG_SERIALIZE_ENUM_JSON_SIMPLE(TextureImportDesc, archive, Filter);
        IG_SERIALIZE_ENUM_JSON_SIMPLE(TextureImportDesc, archive, AddressModeU);
        IG_SERIALIZE_ENUM_JSON_SIMPLE(TextureImportDesc, archive, AddressModeV);
        IG_SERIALIZE_ENUM_JSON_SIMPLE(TextureImportDesc, archive, AddressModeW);
        return archive;
    }

    const Json& TextureImportDesc::Deserialize(const Json& archive)
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

    Json& TextureLoadDesc::Serialize(Json& archive) const
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

    const Json& TextureLoadDesc::Deserialize(const Json& archive)
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

    Texture::Texture(RenderContext& renderContext, const Desc& snapshot, const RenderResource<GpuTexture> gpuTexture, const RenderResource<GpuView> srv, const RenderResource<GpuView> sampler)
        : renderContext(&renderContext), snapshot(snapshot), gpuTexture(gpuTexture), srv(srv), sampler(sampler)
    {
        IG_CHECK(gpuTexture);
        IG_CHECK(srv);
        IG_CHECK(sampler);
    }

    Texture::~Texture()
    {
        if (renderContext != nullptr)
        {
            renderContext->DestroyTexture(gpuTexture);
            renderContext->DestroyGpuView(srv);
            renderContext->DestroyGpuView(sampler);
        }
    }
}    // namespace ig

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
}    // namespace ig::details
