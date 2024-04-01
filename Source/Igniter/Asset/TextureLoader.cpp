#include <Igniter.h>
#include <Core/Timer.h>
#include <Core/Log.h>
#include <Core/Serializable.h>
#include <Core/TypeUtils.h>
#include <Filesystem/Utils.h>
#include <D3D12/RenderDevice.h>
#include <D3D12/GpuTexture.h>
#include <D3D12/GpuView.h>
#include <Render/GpuUploader.h>
#include <Render/GpuViewManager.h>

#include <Asset/TextureLoader.h>

IG_DEFINE_LOG_CATEGORY(TextureLoader);

namespace ig
{
    TextureLoader::TextureLoader(HandleManager& handleManager, RenderDevice& renderDevice, GpuUploader& gpuUploader, GpuViewManager& gpuViewManager) : handleManager(handleManager),
                                                                                                                                                       renderDevice(renderDevice),
                                                                                                                                                       gpuUploader(gpuUploader),
                                                                                                                                                       gpuViewManager(gpuViewManager)
    {
    }

    Result<Texture, ETextureLoaderStatus> TextureLoader::Load(const Texture::Desc& desc)
    {
        const AssetInfo& assetInfo{ desc.Info };
        if (!assetInfo.IsValid())
        {
            return MakeFail<Texture, ETextureLoaderStatus::InvalidAssetInfo>();
        }

        if (assetInfo.Type != EAssetType::Texture)
        {
            return MakeFail<Texture, ETextureLoaderStatus::AssetTypeMismatch>();
        }

        /* Check TextureLoadDesc data */
        const Texture::LoadDesc& loadDesc{ desc.LoadDescriptor };
        if (loadDesc.Width == 0 || loadDesc.Height == 0 || loadDesc.DepthOrArrayLength == 0 || loadDesc.Mips == 0)
        {
            return MakeFail<Texture, ETextureLoaderStatus::InvalidDimensions>();
        }

        if (loadDesc.Format == DXGI_FORMAT_UNKNOWN)
        {
            return MakeFail<Texture, ETextureLoaderStatus::UnknownFormat>();
        }

        /* Load asset from file */
        const fs::path assetPath = MakeAssetPath(EAssetType::Texture, assetInfo.Guid);
        if (!fs::exists(assetPath))
        {
            return MakeFail<Texture, ETextureLoaderStatus::FileDoesNotExists>();
        }

        DirectX::TexMetadata ddsMeta;
        DirectX::ScratchImage scratchImage;
        HRESULT res = DirectX::LoadFromDDSFile(assetPath.c_str(), DirectX::DDS_FLAGS_NONE, &ddsMeta, scratchImage);
        if (FAILED(res))
        {
            return MakeFail<Texture, ETextureLoaderStatus::FailedLoadFromFile>();
        }

        /* Check metadata mismatch */
        if (loadDesc.Width != ddsMeta.width ||
            loadDesc.Height != ddsMeta.height ||
            (loadDesc.DepthOrArrayLength != ddsMeta.depth && loadDesc.DepthOrArrayLength != ddsMeta.arraySize))
        {
            return MakeFail<Texture, ETextureLoaderStatus::DimensionsMismatch>();
        }

        if (loadDesc.bIsCubemap != loadDesc.bIsCubemap)
        {
            return MakeFail<Texture, ETextureLoaderStatus::CubemapFlagMismatch>();
        }

        if (loadDesc.bIsCubemap && (loadDesc.DepthOrArrayLength % 6 == 0))
        {
            return MakeFail<Texture, ETextureLoaderStatus::InvalidCubemapArrayLength>();
        }

        if (loadDesc.Dimension != details::AsTexDimension(ddsMeta.dimension))
        {
            return MakeFail<Texture, ETextureLoaderStatus::DimensionFlagMismatch>();
        }

        if (loadDesc.Format != ddsMeta.format)
        {
            return MakeFail<Texture, ETextureLoaderStatus::FormatMismatch>();
        }

        /* Configure Texture Description */
        /* #sy_todo Support MSAA */
        GPUTextureDesc texDesc{};
        if (loadDesc.bIsCubemap)
        {
            /* #sy_todo Support Cubemap Array */
            texDesc.AsCubemap(loadDesc.Width, loadDesc.Height,
                              loadDesc.Mips,
                              loadDesc.Format);
        }
        else if (loadDesc.Dimension == ETextureDimension::Tex1D)
        {
            if (loadDesc.IsArray())
            {
                texDesc.AsTexture1DArray(loadDesc.Width, loadDesc.DepthOrArrayLength,
                                         loadDesc.Mips,
                                         loadDesc.Format);
            }
            else
            {
                texDesc.AsTexture1D(loadDesc.Width,
                                    loadDesc.Mips,
                                    loadDesc.Format);
            }
        }
        else if (loadDesc.Dimension == ETextureDimension::Tex2D)
        {
            if (loadDesc.IsArray())
            {
                texDesc.AsTexture2DArray(loadDesc.Width, loadDesc.Height, loadDesc.DepthOrArrayLength,
                                         loadDesc.Mips,
                                         loadDesc.Format);
            }
            else
            {
                texDesc.AsTexture2D(loadDesc.Width, loadDesc.Height,
                                    loadDesc.Mips,
                                    loadDesc.Format);
            }
        }
        else
        {
            IG_CHECK(!loadDesc.IsArray());
            texDesc.AsTexture3D(loadDesc.Width, loadDesc.Height, loadDesc.DepthOrArrayLength,
                                loadDesc.Mips,
                                loadDesc.Format);
        }
        texDesc.DebugName = String(std::format("{}({})", assetInfo.VirtualPath, assetInfo.Guid));
        texDesc.InitialLayout = D3D12_BARRIER_LAYOUT_COMMON;

        /* Create Texture from RenderDevice */
        std::optional<GpuTexture> newTex = renderDevice.CreateTexture(texDesc);
        if (!newTex)
        {
            return MakeFail<Texture, ETextureLoaderStatus::FailedCreateTexture>();
        }

        /* Upload texture subresources from sysram to vram */
        const size_t numSubresources = scratchImage.GetImageCount();
        std::vector<D3D12_SUBRESOURCE_DATA> subresources(numSubresources);
        const DirectX::Image* images = scratchImage.GetImages();

        for (size_t idx = 0; idx < numSubresources; ++idx)
        {
            D3D12_SUBRESOURCE_DATA& subresource = subresources[idx];
            subresource.RowPitch = images[idx].rowPitch;
            subresource.SlicePitch = images[idx].slicePitch;
            subresource.pData = images[idx].pixels;
        }

        // COMMON Layout 인 상태에서 텍스처가 GPU 메모리 상에서 어떻게 배치되어있는지
        const GpuCopyableFootprints destCopyableFootprints = renderDevice.GetCopyableFootprints(texDesc, 0, static_cast<uint32_t>(numSubresources), 0);
        UploadContext texUploadCtx = gpuUploader.Reserve(destCopyableFootprints.RequiredSize);

        /* Write subresources to upload buffer */
        for (uint32_t idx = 0; idx < numSubresources; ++idx)
        {
            const D3D12_SUBRESOURCE_DATA& srcSubresource = subresources[idx];
            const D3D12_SUBRESOURCE_FOOTPRINT& dstFootprint = destCopyableFootprints.Layouts[idx].Footprint;
            const size_t rowSizesInBytes = destCopyableFootprints.RowSizesInBytes[idx];
            for (uint32_t z = 0; z < dstFootprint.Depth; ++z)
            {
                const size_t dstSlicePitch = static_cast<size_t>(dstFootprint.RowPitch) * destCopyableFootprints.NumRows[idx];
                const size_t dstSliceOffset = dstSlicePitch * z;
                const size_t srcSliceOffset = srcSubresource.SlicePitch * z;
                for (uint32_t y = 0; y < destCopyableFootprints.NumRows[idx]; ++y)
                {
                    const size_t dstRowOffset = static_cast<size_t>(dstFootprint.RowPitch) * y;
                    const size_t srcRowOffset = srcSubresource.RowPitch * y;

                    const size_t dstOffset = destCopyableFootprints.Layouts[idx].Offset + dstSliceOffset + dstRowOffset;
                    const size_t srcOffset = srcSliceOffset + srcRowOffset;
                    texUploadCtx.WriteData(reinterpret_cast<const uint8_t*>(srcSubresource.pData),
                                           srcOffset, dstOffset,
                                           rowSizesInBytes);
                }
            }

            texUploadCtx.CopyTextureRegion(0, *newTex, idx, destCopyableFootprints.Layouts[idx]);
        }

        std::optional<GpuSync> texUploadSync = gpuUploader.Submit(texUploadCtx);
        IG_CHECK(texUploadSync);
        texUploadSync->WaitOnCpu();

        /* Create Shader Resource View (GpuView) */
        Handle<GpuView, GpuViewManager*> srv = gpuViewManager.RequestShaderResourceView(
            *newTex,
            D3D12_TEX2D_SRV{
                .MostDetailedMip = 0,
                .MipLevels = IG_NUMERIC_MAX_OF(D3D12_TEX2D_SRV::MipLevels),
                .PlaneSlice = 0,
                .ResourceMinLODClamp = 0.f });

        if (!srv)
        {
            return MakeFail<Texture, ETextureLoaderStatus::FailedCreateShaderResourceView>();
        }

        auto samplerView = gpuViewManager.RequestSampler(D3D12_SAMPLER_DESC{
            .Filter = loadDesc.Filter,
            .AddressU = loadDesc.AddressModeU,
            .AddressV = loadDesc.AddressModeV,
            .AddressW = loadDesc.AddressModeW,
            .MipLODBias = 0.f,
            /* #sy_todo if filter ==  anisotropic, Add MaxAnisotropy at load config & import config */
            .MaxAnisotropy = ((loadDesc.Filter == D3D12_FILTER_ANISOTROPIC || loadDesc.Filter == D3D12_FILTER_COMPARISON_ANISOTROPIC) ? 1u : 0u),
            .ComparisonFunc = D3D12_COMPARISON_FUNC_NONE /* #sy_todo if filter == comparison, setup comparison func */,
            .BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK,
            .MinLOD = 0.f,
            .MaxLOD = 0.f });

        if (!samplerView)
        {
            return MakeFail<Texture, ETextureLoaderStatus::FailedCreateSamplerView>();
        }

        /* #sy_todo Layout transition COMMON -> SHADER_RESOURCE? */

        return MakeSuccess<Texture, ETextureLoaderStatus>(Texture{
            { assetInfo, loadDesc },
            { handleManager, std::move(newTex.value()) },
            std::move(srv),
            samplerView });
    }
}