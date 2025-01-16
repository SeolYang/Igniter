#include "Igniter/Igniter.h"
#include "Igniter/Core/Timer.h"
#include "Igniter/Core/Log.h"
#include "Igniter/Core/ContainerUtils.h"
#include "Igniter/Core/FrameManager.h"
#include "Igniter/Filesystem/Utils.h"
#include "Igniter/D3D12/GpuDevice.h"
#include "Igniter/D3D12/GpuTexture.h"
#include "Igniter/D3D12/GpuView.h"
#include "Igniter/D3D12/CommandList.h"
#include "Igniter/Render/GpuUploader.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Asset/TextureLoader.h"

IG_DECLARE_LOG_CATEGORY(TextureLoader);

namespace ig
{
    TextureLoader::TextureLoader(RenderContext& renderContext)
        : renderContext(renderContext)
    {
    }

    Result<Texture, ETextureLoaderStatus> TextureLoader::Load(const Texture::Desc& desc)
    {
        const AssetInfo& assetInfo{desc.Info};
        if (!assetInfo.IsValid())
        {
            return MakeFail<Texture, ETextureLoaderStatus::InvalidAssetInfo>();
        }

        if (assetInfo.GetCategory() != EAssetCategory::Texture)
        {
            return MakeFail<Texture, ETextureLoaderStatus::AssetTypeMismatch>();
        }

        /* Check TextureLoadDesc data */
        const Texture::LoadDesc& loadDesc{desc.LoadDescriptor};
        if (loadDesc.Width == 0 || loadDesc.Height == 0 || loadDesc.DepthOrArrayLength == 0 || loadDesc.Mips == 0)
        {
            return MakeFail<Texture, ETextureLoaderStatus::InvalidDimensions>();
        }

        if (loadDesc.Format == DXGI_FORMAT_UNKNOWN)
        {
            return MakeFail<Texture, ETextureLoaderStatus::UnknownFormat>();
        }

        /* Load asset from file */
        const Path assetPath = MakeAssetPath(EAssetCategory::Texture, assetInfo.GetGuid());
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
        if (loadDesc.Width != ddsMeta.width || loadDesc.Height != ddsMeta.height ||
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
        GpuTextureDesc texDesc{};
        if (loadDesc.bIsCubemap)
        {
            /* #sy_todo Support Cubemap Array */
            texDesc.AsCubemap(loadDesc.Width, loadDesc.Height, loadDesc.Mips, loadDesc.Format);
        }
        else if (loadDesc.Dimension == ETextureDimension::Tex1D)
        {
            if (loadDesc.IsArray())
            {
                texDesc.AsTexture1DArray(loadDesc.Width, loadDesc.DepthOrArrayLength, loadDesc.Mips, loadDesc.Format);
            }
            else
            {
                texDesc.AsTexture1D(loadDesc.Width, loadDesc.Mips, loadDesc.Format);
            }
        }
        else if (loadDesc.Dimension == ETextureDimension::Tex2D)
        {
            if (loadDesc.IsArray())
            {
                texDesc.AsTexture2DArray(loadDesc.Width, loadDesc.Height, loadDesc.DepthOrArrayLength, loadDesc.Mips, loadDesc.Format);
            }
            else
            {
                texDesc.AsTexture2D(loadDesc.Width, loadDesc.Height, loadDesc.Mips, loadDesc.Format);
            }
        }
        else
        {
            IG_CHECK(!loadDesc.IsArray());
            texDesc.AsTexture3D(loadDesc.Width, loadDesc.Height, loadDesc.DepthOrArrayLength, loadDesc.Mips, loadDesc.Format);
        }
        texDesc.DebugName = String(std::format("{}({})", assetInfo.GetVirtualPath(), assetInfo.GetGuid()));
        texDesc.InitialLayout = D3D12_BARRIER_LAYOUT_COMMON;

        /* Create Texture from GpuDevice */
        const RenderHandle<GpuTexture> newTexture = renderContext.CreateTexture(texDesc);
        if (!newTexture)
        {
            return MakeFail<Texture, ETextureLoaderStatus::FailedCreateTexture>();
        }

        /* Upload texture subresources from sysram to vram */
        const Size numSubresources = scratchImage.GetImageCount();
        Vector<D3D12_SUBRESOURCE_DATA> subresources(numSubresources);
        const DirectX::Image* images = scratchImage.GetImages();

        for (Size idx = 0; idx < numSubresources; ++idx)
        {
            D3D12_SUBRESOURCE_DATA& subresource = subresources[idx];
            subresource.RowPitch = images[idx].rowPitch;
            subresource.SlicePitch = images[idx].slicePitch;
            subresource.pData = images[idx].pixels;
        }

        // COMMON Layout 인 상태에서 텍스처가 GPU 메모리 상에서 어떻게 배치되어있는지
        GpuUploader& gpuUploader{renderContext.GetGpuUploader()};

        const GpuCopyableFootprints destCopyableFootprints =
            renderContext.GetGpuDevice().GetCopyableFootprints(texDesc, 0, static_cast<U32>(numSubresources), 0);
        UploadContext texUploadCtx = gpuUploader.Reserve(destCopyableFootprints.RequiredSize);

        GpuTexture* newTexturePtr = renderContext.Lookup(newTexture);
        IG_CHECK(newTexturePtr != nullptr);
        texUploadCtx.CopyTextureSimple(*newTexturePtr, destCopyableFootprints, subresources);

        std::optional<GpuSyncPoint> texUploadSync = gpuUploader.Submit(texUploadCtx);
        IG_CHECK(texUploadSync);
        texUploadSync->WaitOnCpu();

        CommandQueue& mainGfxQueue = renderContext.GetMainGfxQueue();
        GpuFence& mainGfxFence = renderContext.GetMainGfxFence();
        auto cmdList = renderContext.GetMainGfxCommandListPool().Request(FrameManager::GetLocalFrameIndex(), "BarrierAfterUpload_TexUpload"_fs);
        {
            cmdList->Open();
            cmdList->AddPendingTextureBarrier(*newTexturePtr,
                                              D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_NONE,
                                              D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_NO_ACCESS,
                                              D3D12_BARRIER_LAYOUT_COMMON, D3D12_BARRIER_LAYOUT_SHADER_RESOURCE);
            cmdList->FlushBarriers();
            cmdList->Close();
        }
        CommandList* cmdLists[1] = {(CommandList*)cmdList};
        mainGfxQueue.ExecuteCommandLists(cmdLists);
        GpuSyncPoint barrierSync{mainGfxQueue.MakeSyncPointWithSignal(mainGfxFence)};
        barrierSync.WaitOnCpu();

        const RenderHandle<GpuView> srv = renderContext.CreateShaderResourceView(newTexture,
                                                                                 D3D12_TEX2D_SRV{
                                                                                     .MostDetailedMip = 0, .MipLevels = IG_NUMERIC_MAX_OF(D3D12_TEX2D_SRV::MipLevels), .PlaneSlice = 0, .ResourceMinLODClamp = 0.f});
        if (!srv)
        {
            return MakeFail<Texture, ETextureLoaderStatus::FailedCreateShaderResourceView>();
        }

        const RenderHandle<GpuView> samplerView = renderContext.CreateSamplerView(D3D12_SAMPLER_DESC{
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
            .MaxLOD = 0.f});
        if (!samplerView)
        {
            return MakeFail<Texture, ETextureLoaderStatus::FailedCreateSamplerView>();
        }

        /* #sy_todo Layout transition COMMON -> SHADER_RESOURCE? */
        return MakeSuccess<Texture, ETextureLoaderStatus>(Texture{renderContext, desc, newTexture, srv, samplerView});
    }

    Result<Texture, details::EMakeDefaultTexStatus> TextureLoader::MakeDefault(const AssetInfo& assetInfo)
    {
        constexpr DXGI_FORMAT Format{DXGI_FORMAT_R8G8B8A8_UNORM};
        constexpr LONG_PTR BytesPerPixel{4};
        constexpr LONG_PTR BlockSizeInPixels{8};
        constexpr LONG_PTR NumBlocksPerAxis{16};
        constexpr uint8_t BrightPixelElement{220};
        constexpr uint8_t DarkPixelElement{127};

        constexpr LONG_PTR Width{BlockSizeInPixels * NumBlocksPerAxis};
        constexpr LONG_PTR Height{BlockSizeInPixels * NumBlocksPerAxis};
        constexpr LONG_PTR NumPixels{Width * Height};
        Vector<uint8_t> bytes(NumPixels * BytesPerPixel);
        bool bUseBrightPixel = true;
        U32 pixelCounter{0};
        for (LONG_PTR pixelIdx = 0; pixelIdx < NumPixels; ++pixelIdx)
        {
            const LONG_PTR offset{pixelIdx * BytesPerPixel};
            bytes[offset + 0] = bUseBrightPixel ? BrightPixelElement : DarkPixelElement;
            bytes[offset + 1] = bUseBrightPixel ? BrightPixelElement : DarkPixelElement;
            bytes[offset + 2] = bUseBrightPixel ? BrightPixelElement : DarkPixelElement;
            bytes[offset + 3] = 255;

            ++pixelCounter;
            bUseBrightPixel = (pixelCounter % BlockSizeInPixels == 0) ? !bUseBrightPixel : bUseBrightPixel;

            bUseBrightPixel = (pixelCounter % (BlockSizeInPixels * BlockSizeInPixels * NumBlocksPerAxis) == 0) ? !bUseBrightPixel : bUseBrightPixel;
        }

        GpuTextureDesc texDesc{};
        texDesc.AsTexture2D(Width, Height, 1, Format);
        texDesc.DebugName = String(assetInfo.GetVirtualPath());
        RenderHandle<GpuTexture> newTexture{renderContext.CreateTexture(texDesc)};
        if (!newTexture)
        {
            return MakeFail<Texture, details::EMakeDefaultTexStatus::FailedCreateTexture>();
        }

        constexpr LONG_PTR RowPitch{Width * BytesPerPixel};
        constexpr LONG_PTR SlicePitch{Height * RowPitch};
        IG_CHECK(SlicePitch == bytes.size());

        const D3D12_SUBRESOURCE_DATA subresource{
            .pData = reinterpret_cast<const void*>(bytes.data()),
            .RowPitch = RowPitch,
            .SlicePitch = SlicePitch,
        };

        GpuUploader& gpuUploader{renderContext.GetGpuUploader()};
        const GpuCopyableFootprints dstCopyableFootprints{renderContext.GetGpuDevice().GetCopyableFootprints(texDesc, 0, 1, 0)};
        UploadContext uploadCtx{gpuUploader.Reserve(dstCopyableFootprints.RequiredSize)};
        GpuTexture* newTexturePtr = renderContext.Lookup(newTexture);
        IG_CHECK(newTexturePtr != nullptr);
        Vector<D3D12_SUBRESOURCE_DATA> subresources{subresource};
        uploadCtx.CopyTextureSimple(*newTexturePtr, dstCopyableFootprints, subresources);
        std::optional<GpuSyncPoint> sync{gpuUploader.Submit(uploadCtx)};
        IG_CHECK(sync);
        sync->WaitOnCpu();

        CommandQueue& mainGfxQueue = renderContext.GetMainGfxQueue();
        GpuFence& mainGfxFence = renderContext.GetMainGfxFence();
        auto cmdList = renderContext.GetMainGfxCommandListPool().Request(FrameManager::GetLocalFrameIndex(), "BarrierAfter_TexUpload"_fs);
        {
            cmdList->Open();
            cmdList->AddPendingTextureBarrier(*newTexturePtr,
                                              D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_NONE,
                                              D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_NO_ACCESS,
                                              D3D12_BARRIER_LAYOUT_COMMON, D3D12_BARRIER_LAYOUT_SHADER_RESOURCE);
            cmdList->FlushBarriers();
            cmdList->Close();
        }
        CommandList* cmdLists[1] = {(CommandList*)cmdList};
        mainGfxQueue.ExecuteCommandLists(cmdLists);
        GpuSyncPoint barrierSync{mainGfxQueue.MakeSyncPointWithSignal(mainGfxFence)};
        barrierSync.WaitOnCpu();

        RenderHandle<GpuView> srv = renderContext.CreateShaderResourceView(newTexture,
                                                                           D3D12_TEX2D_SRV{
                                                                               .MostDetailedMip = 0, .MipLevels = IG_NUMERIC_MAX_OF(D3D12_TEX2D_SRV::MipLevels), .PlaneSlice = 0, .ResourceMinLODClamp = 0.f});
        if (!srv)
        {
            return MakeFail<Texture, details::EMakeDefaultTexStatus::FailedCreateShaderResourceView>();
        }

        const Texture::LoadDesc loadDesc{
            .Format = Format,
            .Width = Width,
            .Height = Height,
            .Filter = D3D12_FILTER_MIN_MAG_MIP_POINT,
        };

        const RenderHandle<GpuView> samplerView = renderContext.CreateSamplerView(
            D3D12_SAMPLER_DESC{
                .Filter = loadDesc.Filter,
                .AddressU = loadDesc.AddressModeU,
                .AddressV = loadDesc.AddressModeV,
                .AddressW = loadDesc.AddressModeW,
                .MipLODBias = 0.f,
                .MaxAnisotropy = 0,
                .ComparisonFunc = D3D12_COMPARISON_FUNC_NONE,
                .BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK,
                .MinLOD = 0.f,
                .MaxLOD = 0.f});
        if (!samplerView)
        {
            return MakeFail<Texture, details::EMakeDefaultTexStatus::FailedCreateSamplerView>();
        }

        return MakeSuccess<Texture, details::EMakeDefaultTexStatus>(
            Texture{renderContext, Texture::Desc{.Info = assetInfo, .LoadDescriptor = loadDesc}, newTexture, srv, samplerView});
    }

    Result<Texture, details::EMakeDefaultTexStatus> TextureLoader::MakeMonochrome(const AssetInfo& assetInfo, const Color& color)
    {
        constexpr DXGI_FORMAT Format{DXGI_FORMAT_R8G8B8A8_UNORM};
        constexpr LONG_PTR BytesPerPixel{4};

        constexpr LONG_PTR Width{1};
        constexpr LONG_PTR Height{1};
        constexpr LONG_PTR NumPixels{Width * Height};
        Vector<uint8_t> bytes(NumPixels * BytesPerPixel);

        Color saturatedColor{};
        color.Saturate(saturatedColor);
        bytes[0] = static_cast<uint8_t>(saturatedColor.R() * 255);
        bytes[1] = static_cast<uint8_t>(saturatedColor.G() * 255);
        bytes[2] = static_cast<uint8_t>(saturatedColor.B() * 255);
        bytes[3] = 255;

        GpuTextureDesc texDesc{};
        texDesc.AsTexture2D(Width, Height, 1, Format);
        texDesc.DebugName = String(assetInfo.GetVirtualPath());
        const RenderHandle<GpuTexture> newTexture{renderContext.CreateTexture(texDesc)};
        if (!newTexture)
        {
            return MakeFail<Texture, details::EMakeDefaultTexStatus::FailedCreateTexture>();
        }

        GpuTexture* newTexturePtr{renderContext.Lookup(newTexture)};
        constexpr LONG_PTR RowPitch{Width * BytesPerPixel};
        constexpr LONG_PTR SlicePitch{Height * RowPitch};
        IG_CHECK(SlicePitch == bytes.size());

        const D3D12_SUBRESOURCE_DATA subresource{
            .pData = reinterpret_cast<const void*>(bytes.data()),
            .RowPitch = RowPitch,
            .SlicePitch = SlicePitch,
        };
        Vector<D3D12_SUBRESOURCE_DATA> subresources{subresource};

        GpuUploader& gpuUploader{renderContext.GetGpuUploader()};
        const GpuCopyableFootprints dstCopyableFootprints{renderContext.GetGpuDevice().GetCopyableFootprints(texDesc, 0, 1, 0)};
        UploadContext uploadCtx{gpuUploader.Reserve(dstCopyableFootprints.RequiredSize)};
        uploadCtx.CopyTextureSimple(*newTexturePtr, dstCopyableFootprints, subresources);
        std::optional<GpuSyncPoint> sync{gpuUploader.Submit(uploadCtx)};
        IG_CHECK(sync);
        sync->WaitOnCpu();

        CommandQueue& mainGfxQueue = renderContext.GetMainGfxQueue();
        GpuFence& mainGfxFence = renderContext.GetMainGfxFence();
        auto cmdList = renderContext.GetMainGfxCommandListPool().Request(FrameManager::GetLocalFrameIndex(), "BarrierAfter_TexUpload"_fs);
        {
            cmdList->Open();
            cmdList->AddPendingTextureBarrier(*newTexturePtr,
                                              D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_NONE,
                                              D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_NO_ACCESS,
                                              D3D12_BARRIER_LAYOUT_COMMON, D3D12_BARRIER_LAYOUT_SHADER_RESOURCE);
            cmdList->FlushBarriers();
            cmdList->Close();
        }

        CommandList* cmdLists[1]{(CommandList*)cmdList};
        mainGfxQueue.ExecuteCommandLists(cmdLists);
        GpuSyncPoint barrierSync{mainGfxQueue.MakeSyncPointWithSignal(mainGfxFence)};
        barrierSync.WaitOnCpu();

        const RenderHandle<GpuView> srv = renderContext.CreateShaderResourceView(newTexture,
                                                                                 D3D12_TEX2D_SRV{
                                                                                     .MostDetailedMip = 0,
                                                                                     .MipLevels = IG_NUMERIC_MAX_OF(D3D12_TEX2D_SRV::MipLevels),
                                                                                     .PlaneSlice = 0,
                                                                                     .ResourceMinLODClamp = 0.f});

        if (!srv)
        {
            return MakeFail<Texture, details::EMakeDefaultTexStatus::FailedCreateShaderResourceView>();
        }

        const Texture::LoadDesc loadDesc{
            .Format = Format,
            .Width = Width,
            .Height = Height,
            .Filter = D3D12_FILTER_MIN_MAG_MIP_POINT,
        };

        auto samplerView = renderContext.CreateSamplerView(D3D12_SAMPLER_DESC{
            .Filter = loadDesc.Filter,
            .AddressU = loadDesc.AddressModeU,
            .AddressV = loadDesc.AddressModeV,
            .AddressW = loadDesc.AddressModeW,
            .MipLODBias = 0.f,
            .MaxAnisotropy = 0,
            .ComparisonFunc = D3D12_COMPARISON_FUNC_NONE,
            .BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK,
            .MinLOD = 0.f,
            .MaxLOD = 0.f});

        if (!samplerView)
        {
            return MakeFail<Texture, details::EMakeDefaultTexStatus::FailedCreateSamplerView>();
        }

        return MakeSuccess<Texture, details::EMakeDefaultTexStatus>(
            Texture{renderContext, Texture::Desc{.Info = assetInfo, .LoadDescriptor = loadDesc}, newTexture, srv, samplerView});
    }
} // namespace ig
