#include "Igniter/Igniter.h"
#include "Igniter/Core/Log.h"
#include "Igniter/Core/Timer.h"
#include "Igniter/Core/Serialization.h"
#include "Igniter/Core/ComInitializer.h"
#include "Igniter/Filesystem/Utils.h"
#include "Igniter/Asset/TextureImporter.h"

IG_DECLARE_LOG_CATEGORY(TextureImporterLog);

IG_DEFINE_LOG_CATEGORY(TextureImporterLog);

namespace ig
{
    static bool IsDDSExtnsion(const Path& extension)
    {
        std::string ext = extension.string();
        std::transform(
            ext.begin(), ext.end(), ext.begin(), [](const char character)
            {
                return static_cast<char>(::tolower(static_cast<int>(character)));
            });

        return ext == ".dds";
    }

    static bool IsWICExtension(const Path& extension)
    {
        std::string ext = extension.string();
        std::transform(
            ext.begin(), ext.end(), ext.begin(), [](const char character)
            {
                return static_cast<char>(::tolower(static_cast<int>(character)));
            });

        return (ext == ".png") || (ext == ".jpg") || (ext == ".jpeg") || (ext == ".bmp") || (ext == ".gif") || (ext == ".tiff");
    }

    static bool IsHDRExtnsion(const Path& extension)
    {
        std::string ext = extension.string();
        std::transform(
            ext.begin(), ext.end(), ext.begin(), [](const char character)
            {
                return static_cast<char>(::tolower(static_cast<int>(character)));
            });

        return (ext == ".hdr");
    }

    static DXGI_FORMAT AsBCnFormat(const ETextureCompressionMode compMode, const DXGI_FORMAT format)
    {
        switch (compMode)
        {
        case ETextureCompressionMode::BC4:
            return IsUnormFormat(format) ? DXGI_FORMAT_BC4_UNORM : DXGI_FORMAT_BC4_SNORM;
        case ETextureCompressionMode::BC5:
            return IsUnormFormat(format) ? DXGI_FORMAT_BC5_UNORM : DXGI_FORMAT_BC5_SNORM;
        case ETextureCompressionMode::BC6H:
            return IsUnsignedFormat(format) ? DXGI_FORMAT_BC6H_UF16 : DXGI_FORMAT_BC6H_SF16;
        case ETextureCompressionMode::BC7:
            return DirectX::IsSRGB(format) ? DXGI_FORMAT_BC7_UNORM_SRGB : DXGI_FORMAT_BC7_UNORM;
        default:
            return DXGI_FORMAT_UNKNOWN;
        }
    }

    TextureImporter::TextureImporter()
    {
        U32 creationFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
        creationFlags |= static_cast<U32>(D3D11_CREATE_DEVICE_DEBUG);
#endif
        const D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_10_0;
        const HRESULT res = D3D11CreateDevice(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, creationFlags, &featureLevel, 1, D3D11_SDK_VERSION, &d3d11Device, nullptr, nullptr);

        if (FAILED(res))
        {
            IG_LOG(TextureImporterLog, Error, "Failed create d3d11 device. HRESULT: {:#X}", res);
        }
    }

    TextureImporter::~TextureImporter()
    {
        if (d3d11Device != nullptr)
        {
            d3d11Device->Release();
        }
    }

    Result<Texture::Desc, ETextureImportStatus> TextureImporter::Import(const std::string_view resPathStr, TextureImportDesc importDesc)
    {
        CoInitializeUnique();

        const Path resPath{resPathStr};
        if (!fs::exists(resPath))
        {
            return MakeFail<Texture::Desc, ETextureImportStatus::FileDoesNotExists>();
        }

        const Path resExtension = resPath.extension();
        DirectX::ScratchImage targetTex{};
        DirectX::TexMetadata texMetadata{};

        HRESULT loadRes = S_OK;
        bool bIsDDSFormat = false;
        bool bIsHDRFormat = false;
        if (IsDDSExtnsion(resExtension))
        {
            // 만약 DDS 타입이면, 바로 가공 없이 에셋으로 사용한다. 이 경우,
            // 제공된 config 은 무시된다.; 애초에 DDS로 미리 가공했는데 다시 압축을 풀고, 밉맵을 생성하고 할 필요가 없음.
            IG_LOG(TextureImporterLog, Warning,
                "For DDS files, the provided configuration values are disregarded, "
                "and the supplied file is used as the asset as it is. File: {}",
                resPathStr);

            loadRes = DirectX::LoadFromDDSFile(resPath.c_str(), DirectX::DDS_FLAGS_NONE, &texMetadata, targetTex);
            bIsDDSFormat = true;
        }
        else if (IsWICExtension(resExtension))
        {
            loadRes = DirectX::LoadFromWICFile(resPath.c_str(), DirectX::WIC_FLAGS_NONE, &texMetadata, targetTex);
        }
        else if (IsHDRExtnsion(resExtension))
        {
            loadRes = DirectX::LoadFromHDRFile(resPath.c_str(), &texMetadata, targetTex);
            bIsHDRFormat = true;
        }
        else
        {
            return MakeFail<Texture::Desc, ETextureImportStatus::UnsupportedExtension>();
        }

        if (FAILED(loadRes))
        {
            return MakeFail<Texture::Desc, ETextureImportStatus::FailedLoadFromFile>();
        }

        if (!bIsDDSFormat)
        {
            const bool bValidDimensions = texMetadata.width > 0 && texMetadata.height > 0 && texMetadata.depth > 0 && texMetadata.arraySize > 0;
            if (!bValidDimensions)
            {
                return MakeFail<Texture::Desc, ETextureImportStatus::InvalidDimensions>();
            }

            const bool bValidVolumemapArgs = !texMetadata.IsVolumemap() || (texMetadata.IsVolumemap() && texMetadata.arraySize == 1);
            if (!bValidVolumemapArgs)
            {
                return MakeFail<Texture::Desc, ETextureImportStatus::InvalidVolumemap>();
            }

            if (texMetadata.format == DXGI_FORMAT_UNKNOWN)
            {
                return MakeFail<Texture::Desc, ETextureImportStatus::UnknownFormat>();
            }

            /* Generate full mipmap chain. */
            if (importDesc.bGenerateMips)
            {
                DirectX::ScratchImage mipChain{};
                const HRESULT genRes = DirectX::GenerateMipMaps(targetTex.GetImages(), targetTex.GetImageCount(), targetTex.GetMetadata(),
                    bIsHDRFormat ? DirectX::TEX_FILTER_FANT : DirectX::TEX_FILTER_LINEAR, 0, mipChain);
                if (FAILED(genRes))
                {
                    return MakeFail<Texture::Desc, ETextureImportStatus::FailedGenerateMips>();
                }

                texMetadata = mipChain.GetMetadata();
                targetTex = std::move(mipChain);
            }

            /* Compress Texture*/
            if (importDesc.CompressionMode != ETextureCompressionMode::None)
            {
                if (bIsHDRFormat && importDesc.CompressionMode != ETextureCompressionMode::BC6H)
                {
                    IG_LOG(TextureImporterLog, Warning,
                        "The compression method for HDR extension textures is "
                        "enforced "
                        "to be the 'BC6H' algorithm. File: {}",
                        resPathStr);

                    importDesc.CompressionMode = ETextureCompressionMode::BC6H;
                }
                else if (IsGreyScaleFormat(texMetadata.format) && importDesc.CompressionMode == ETextureCompressionMode::BC4)
                {
                    IG_LOG(TextureImporterLog, Warning,
                        "The compression method for greyscale-formatted "
                        "textures is "
                        "enforced to be the 'BC4' algorithm. File: {}",
                        resPathStr);

                    importDesc.CompressionMode = ETextureCompressionMode::BC4;
                }

                auto compFlags = static_cast<unsigned long>(DirectX::TEX_COMPRESS_PARALLEL);
                const DXGI_FORMAT compFormat = AsBCnFormat(importDesc.CompressionMode, texMetadata.format);

                const bool bIsGPUCodecAvailable = d3d11Device != nullptr && (importDesc.CompressionMode == ETextureCompressionMode::BC6H ||
                    importDesc.CompressionMode == ETextureCompressionMode::BC7);

                HRESULT compRes = S_FALSE;
                DirectX::ScratchImage compTex{};
                {
                    UniqueLock lock{compressionMutex};
                    if (bIsGPUCodecAvailable)
                    {
                        compRes = DirectX::Compress(d3d11Device, targetTex.GetImages(), targetTex.GetImageCount(), targetTex.GetMetadata(),
                            compFormat, static_cast<DirectX::TEX_COMPRESS_FLAGS>(compFlags), DirectX::TEX_ALPHA_WEIGHT_DEFAULT, compTex);
                    }
                    else
                    {
                        compRes = DirectX::Compress(targetTex.GetImages(), targetTex.GetImageCount(), targetTex.GetMetadata(), compFormat,
                            static_cast<DirectX::TEX_COMPRESS_FLAGS>(compFlags), DirectX::TEX_ALPHA_WEIGHT_DEFAULT, compTex);
                    }
                }

                if (FAILED(compRes))
                {
                    return MakeFail<Texture::Desc, ETextureImportStatus::FailedCompression>();
                }

                texMetadata = compTex.GetMetadata();
                targetTex = std::move(compTex);
            }
        }

        /* Configure Texture Resource Metadata */
        const ResourceInfo resInfo{.Category = EAssetCategory::Texture};
        Json resMetadata{};
        resMetadata << resInfo << importDesc;

        const Path resMetadataPath{MakeResourceMetadataPath(resPath)};
        if (SaveJsonToFile(resMetadataPath, resMetadata))
        {
            IG_LOG(TextureImporterLog, Warning, "Failed to create resource metadata {}.", resMetadataPath.string());
        }

        /* Configure Texture Asset Metadata */
        texMetadata = targetTex.GetMetadata();

        const AssetInfo assetInfo{MakeVirtualPathPreferred(resPath.filename().replace_extension().string()), EAssetCategory::Texture};
        const TextureLoadDesc newLoadConfig{
            .Format = texMetadata.format,
            .Dimension = details::AsTexDimension(texMetadata.dimension),
            .Width = static_cast<U32>(texMetadata.width),
            .Height = static_cast<U32>(texMetadata.height),
            .DepthOrArrayLength = static_cast<uint16_t>(texMetadata.IsVolumemap() ? texMetadata.depth : texMetadata.arraySize),
            .Mips = static_cast<uint16_t>(texMetadata.mipLevels),
            .bIsCubemap = texMetadata.IsCubemap(),
            .Filter = importDesc.Filter,
            .AddressModeU = importDesc.AddressModeU,
            .AddressModeV = importDesc.AddressModeV,
            .AddressModeW = importDesc.AddressModeW
        };

        Json assetMetadata{};
        assetMetadata << assetInfo << newLoadConfig;

        const Path assetMetadataPath = MakeAssetMetadataPath(EAssetCategory::Texture, assetInfo.GetGuid());
        IG_CHECK(!assetMetadataPath.empty());
        if (!SaveJsonToFile(assetMetadataPath, assetMetadata))
        {
            return MakeFail<Texture::Desc, ETextureImportStatus::FailedSaveMetadataToFile>();
        }

        /* Save data to asset file */
        const Path assetPath = MakeAssetPath(EAssetCategory::Texture, assetInfo.GetGuid());
        IG_CHECK(!assetPath.empty());
        if (fs::exists(assetPath))
        {
            IG_LOG(TextureImporterLog, Warning, "The Asset file {} alread exist.", assetPath.string());
        }

        const HRESULT res = DirectX::SaveToDDSFile(
            targetTex.GetImages(), targetTex.GetImageCount(), targetTex.GetMetadata(), DirectX::DDS_FLAGS_NONE, assetPath.c_str());

        if (FAILED(res))
        {
            return MakeFail<Texture::Desc, ETextureImportStatus::FailedSaveAssetToFile>();
        }

        IG_CHECK(assetInfo.IsValid() && assetInfo.GetCategory() == EAssetCategory::Texture);
        return MakeSuccess<Texture::Desc, ETextureImportStatus>(assetInfo, newLoadConfig);
    }
} // namespace ig
