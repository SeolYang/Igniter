#include "Igniter/Igniter.h"
#include "Igniter/Core/Json.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Asset/Texture.h"

namespace ig
{
    Json& TextureImportDesc::Serialize(Json& archive) const
    {
        IG_SERIALIZE_TO_JSON(TextureImportDesc, archive, CompressionMode);
        IG_SERIALIZE_TO_JSON(TextureImportDesc, archive, bGenerateMips);
        IG_SERIALIZE_TO_JSON(TextureImportDesc, archive, Filter);
        IG_SERIALIZE_TO_JSON(TextureImportDesc, archive, AddressModeU);
        IG_SERIALIZE_TO_JSON(TextureImportDesc, archive, AddressModeV);
        IG_SERIALIZE_TO_JSON(TextureImportDesc, archive, AddressModeW);
        return archive;
    }

    const Json& TextureImportDesc::Deserialize(const Json& archive)
    {
        *this = {};
        IG_DESERIALIZE_FROM_JSON_FALLBACK(TextureImportDesc, archive, CompressionMode, ETextureCompressionMode::None);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(TextureImportDesc, archive, bGenerateMips, false);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(TextureImportDesc, archive, Filter, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(TextureImportDesc, archive, AddressModeU, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(TextureImportDesc, archive, AddressModeV, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(TextureImportDesc, archive, AddressModeW, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        return archive;
    }

    Json& TextureLoadDesc::Serialize(Json& archive) const
    {
        IG_SERIALIZE_TO_JSON(TextureLoadDesc, archive, Format);
        IG_SERIALIZE_TO_JSON(TextureLoadDesc, archive, Dimension);
        IG_SERIALIZE_TO_JSON(TextureLoadDesc, archive, Width);
        IG_SERIALIZE_TO_JSON(TextureLoadDesc, archive, Height);
        IG_SERIALIZE_TO_JSON(TextureLoadDesc, archive, DepthOrArrayLength);
        IG_SERIALIZE_TO_JSON(TextureLoadDesc, archive, Mips);
        IG_SERIALIZE_TO_JSON(TextureLoadDesc, archive, bIsCubemap);
        IG_SERIALIZE_TO_JSON(TextureLoadDesc, archive, Filter);
        IG_SERIALIZE_TO_JSON(TextureLoadDesc, archive, AddressModeU);
        IG_SERIALIZE_TO_JSON(TextureLoadDesc, archive, AddressModeV);
        IG_SERIALIZE_TO_JSON(TextureLoadDesc, archive, AddressModeW);
        return archive;
    }

    const Json& TextureLoadDesc::Deserialize(const Json& archive)
    {
        *this = {};
        IG_DESERIALIZE_FROM_JSON_FALLBACK(TextureLoadDesc, archive, Format, DXGI_FORMAT_UNKNOWN);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(TextureLoadDesc, archive, Dimension, ETextureDimension::Tex2D);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(TextureLoadDesc, archive, Width, 0);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(TextureLoadDesc, archive, Height, 0);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(TextureLoadDesc, archive, DepthOrArrayLength, 0);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(TextureLoadDesc, archive, Mips, 0);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(TextureLoadDesc, archive, bIsCubemap, false);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(TextureLoadDesc, archive, Filter, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(TextureLoadDesc, archive, AddressModeU, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(TextureLoadDesc, archive, AddressModeV, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(TextureLoadDesc, archive, AddressModeW, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        return archive;
    }

    Texture::Texture(RenderContext& renderContext, const Desc& snapshot, const Handle<GpuTexture> gpuTexture, const Handle<GpuView> srv, const Handle<GpuView> sampler)
        : renderContext(&renderContext)
        , snapshot(snapshot)
        , gpuTexture(gpuTexture)
        , srv(srv)
        , sampler(sampler)
    {
        IG_CHECK(gpuTexture);
        IG_CHECK(srv);
        IG_CHECK(sampler);
    }

    Texture::~Texture()
    {
        Destroy();
    }

    Texture& Texture::operator=(Texture&& rhs) noexcept
    {
        Destroy();

        renderContext = std::exchange(rhs.renderContext, nullptr);
        snapshot = rhs.snapshot;
        gpuTexture = std::exchange(rhs.gpuTexture, {});
        srv = std::exchange(rhs.srv, {});
        sampler = std::exchange(rhs.sampler, {});

        return *this;
    }

    void Texture::Destroy()
    {
        if (renderContext != nullptr)
        {
            renderContext->DestroyTexture(gpuTexture);
            renderContext->DestroyGpuView(srv);
            renderContext->DestroyGpuView(sampler);
        }

        renderContext = nullptr;
        gpuTexture = {};
        srv = {};
        sampler = {};
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
