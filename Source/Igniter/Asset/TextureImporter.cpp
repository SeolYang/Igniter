#include <Igniter.h>
#include <Core/Log.h>
#include <Core/Timer.h>
#include <Core/ComInitializer.h>
#include <Filesystem/Utils.h>
#include <Asset/TextureImporter.h>

IG_DEFINE_LOG_CATEGORY(TextureImporter);

namespace ig
{
    inline bool IsDDSExtnsion(const fs::path& extension)
    {
        std::string ext = extension.string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](const char character)
                       {
                           return static_cast<char>(::tolower(static_cast<int>(character)));
                       });

        return ext == ".dds";
    }

    inline bool IsWICExtension(const fs::path& extension)
    {
        std::string ext = extension.string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](const char character)
                       {
                           return static_cast<char>(::tolower(static_cast<int>(character)));
                       });

        return (ext == ".png") || (ext == ".jpg") || (ext == ".jpeg") || (ext == ".bmp") || (ext == ".gif") ||
               (ext == ".tiff");
    }

    inline bool IsHDRExtnsion(const fs::path& extension)
    {
        std::string ext = extension.string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](const char character)
                       {
                           return static_cast<char>(::tolower(static_cast<int>(character)));
                       });

        return (ext == ".hdr");
    }

    inline DXGI_FORMAT AsBCnFormat(const ETextureCompressionMode compMode, const DXGI_FORMAT format)
    {
        if (compMode == ETextureCompressionMode::BC4)
        {
            if (IsUnormFormat(format))
            {
                return DXGI_FORMAT_BC4_UNORM;
            }

            return DXGI_FORMAT_BC4_SNORM;
        }
        else if (compMode == ETextureCompressionMode::BC5)
        {
            if (IsUnormFormat(format))
            {
                return DXGI_FORMAT_BC5_UNORM;
            }

            return DXGI_FORMAT_BC5_SNORM;
        }
        else if (compMode == ETextureCompressionMode::BC6H)
        {
            // #sy_todo 정확히 HDR 파일을 읽어왔을 때, 어떤 포맷으로 들어오는지 확인 필
            if (IsUnsignedFormat(format))
            {
                return DXGI_FORMAT_BC6H_UF16;
            }

            return DXGI_FORMAT_BC6H_SF16;
        }
        else if (compMode == ETextureCompressionMode::BC7)
        {
            return DXGI_FORMAT_BC7_UNORM;
        }

        return DXGI_FORMAT_UNKNOWN;
    }

    TextureImporter::TextureImporter()
    {
        uint32_t creationFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
        creationFlags |= static_cast<uint32_t>(D3D11_CREATE_DEVICE_DEBUG);
#endif
        const D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_10_0;
        const HRESULT res = D3D11CreateDevice(nullptr,
                                              D3D_DRIVER_TYPE_HARDWARE, nullptr,
                                              creationFlags, &featureLevel, 1, D3D11_SDK_VERSION,
                                              &d3d11Device, nullptr, nullptr);

        if (FAILED(res))
        {
            IG_LOG(TextureImporter, Error, "Failed create d3d11 device. HRESULT: {:#X}", res);
        }
    }

    TextureImporter::~TextureImporter()
    {
        if (d3d11Device != nullptr)
        {
            d3d11Device->Release();
        }
    }

    Result<Texture::Desc, ETextureImportStatus> TextureImporter::Import(const String resPathStr, TextureImportDesc importDesc)
    {
        CoInitializeUnique();

        const fs::path resPath{ resPathStr.ToStringView() };
        if (!fs::exists(resPath))
        {
            return MakeFail<Texture::Desc, ETextureImportStatus::FileDoesNotExists>();
        }

        const fs::path resExtension = resPath.extension();
        DirectX::ScratchImage targetTex{};
        DirectX::TexMetadata texMetadata{};

        HRESULT loadRes = S_OK;
        bool bIsDDSFormat = false;
        bool bIsHDRFormat = false;
        if (IsDDSExtnsion(resExtension))
        {
            // 만약 DDS 타입이면, 바로 가공 없이 에셋으로 사용한다. 이 경우,
            // 제공된 config 은 무시된다.; 애초에 DDS로 미리 가공했는데 다시 압축을 풀고, 밉맵을 생성하고 할 필요가 없음.
            IG_LOG(TextureImporter, Warning,
                   "For DDS files, the provided configuration values are disregarded, "
                   "and the supplied file is used as the asset as it is. File: {}",
                   resPathStr.ToStringView());

            loadRes = DirectX::LoadFromDDSFile(resPath.c_str(),
                                               DirectX::DDS_FLAGS_NONE,
                                               &texMetadata, targetTex);
            bIsDDSFormat = true;
        }
        else if (IsWICExtension(resExtension))
        {
            loadRes = DirectX::LoadFromWICFile(resPath.c_str(),
                                               DirectX::WIC_FLAGS_FORCE_LINEAR,
                                               &texMetadata, targetTex);

            IG_CHECK(!DirectX::IsSRGB(texMetadata.format));
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
            const bool bValidDimensions = texMetadata.width > 0 && texMetadata.height > 0 && texMetadata.depth > 0 &&
                                          texMetadata.arraySize > 0;
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
                const HRESULT genRes = DirectX::GenerateMipMaps(targetTex.GetImages(), targetTex.GetImageCount(),
                                                                targetTex.GetMetadata(),
                                                                bIsHDRFormat ? DirectX::TEX_FILTER_FANT : DirectX::TEX_FILTER_LINEAR,
                                                                0, mipChain);
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
                    IG_LOG(TextureImporter, Warning,
                           "The compression method for HDR extension textures is "
                           "enforced "
                           "to be the 'BC6H' algorithm. File: {}",
                           resPathStr.ToStringView());

                    importDesc.CompressionMode = ETextureCompressionMode::BC6H;
                }
                else if (IsGreyScaleFormat(texMetadata.format) &&
                         importDesc.CompressionMode == ETextureCompressionMode::BC4)
                {
                    IG_LOG(TextureImporter, Warning,
                           "The compression method for greyscale-formatted "
                           "textures is "
                           "enforced to be the 'BC4' algorithm. File: {}",
                           resPathStr.ToStringView());

                    importDesc.CompressionMode = ETextureCompressionMode::BC4;
                }

                auto compFlags = static_cast<unsigned long>(DirectX::TEX_COMPRESS_PARALLEL);
                const DXGI_FORMAT compFormat = AsBCnFormat(importDesc.CompressionMode, texMetadata.format);

                const bool bIsGPUCodecAvailable = d3d11Device != nullptr &&
                                                  (importDesc.CompressionMode == ETextureCompressionMode::BC6H ||
                                                   importDesc.CompressionMode == ETextureCompressionMode::BC7);

                HRESULT compRes = S_FALSE;
                DirectX::ScratchImage compTex{};
                {
                    UniqueLock lock{ compressionMutex };
                    if (bIsGPUCodecAvailable)
                    {
                        compRes = DirectX::Compress(
                            d3d11Device,
                            targetTex.GetImages(), targetTex.GetImageCount(), targetTex.GetMetadata(),
                            compFormat,
                            static_cast<DirectX::TEX_COMPRESS_FLAGS>(compFlags),
                            DirectX::TEX_ALPHA_WEIGHT_DEFAULT,
                            compTex);
                    }
                    else
                    {
                        compRes = DirectX::Compress(
                            targetTex.GetImages(), targetTex.GetImageCount(),
                            targetTex.GetMetadata(),
                            compFormat,
                            static_cast<DirectX::TEX_COMPRESS_FLAGS>(compFlags),
                            DirectX::TEX_ALPHA_WEIGHT_DEFAULT,
                            compTex);
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
        const ResourceInfo resInfo{ .Type = EAssetType::Texture };
        json resMetadata{};
        resMetadata << resInfo << importDesc;

        const fs::path resMetadataPath{ MakeResourceMetadataPath(resPath) };
        if (SaveJsonToFile(resMetadataPath, resMetadata))
        {
            IG_LOG(TextureImporter, Warning, "Failed to create resource metadata {}.", resMetadataPath.string());
        }

        /* Configure Texture Asset Metadata */
        texMetadata = targetTex.GetMetadata();

        const AssetInfo assetInfo{ MakeVirtualPathPreferred(String(resPath.filename().replace_extension().string())),
                                   EAssetType::Texture,
                                   EAssetPersistency::Default };

        const TextureLoadDesc newLoadConfig{ .Format = texMetadata.format,
                                             .Dimension = details::AsTexDimension(texMetadata.dimension),
                                             .Width = static_cast<uint32_t>(texMetadata.width),
                                             .Height = static_cast<uint32_t>(texMetadata.height),
                                             .DepthOrArrayLength = static_cast<uint16_t>(texMetadata.IsVolumemap() ?
                                                                                             texMetadata.depth :
                                                                                             texMetadata.arraySize),
                                             .Mips = static_cast<uint16_t>(texMetadata.mipLevels),
                                             .bIsCubemap = texMetadata.IsCubemap(),
                                             .Filter = importDesc.Filter,
                                             .AddressModeU = importDesc.AddressModeU,
                                             .AddressModeV = importDesc.AddressModeV,
                                             .AddressModeW = importDesc.AddressModeW };

        json assetMetadata{};
        assetMetadata << assetInfo << newLoadConfig;

        const fs::path assetMetadataPath = MakeAssetMetadataPath(EAssetType::Texture, assetInfo.GetGuid());
        IG_CHECK(!assetMetadataPath.empty());
        if (!SaveJsonToFile(assetMetadataPath, assetMetadata))
        {
            return MakeFail<Texture::Desc, ETextureImportStatus::FailedSaveMetadataToFile>();
        }

        /* Save data to asset file */
        const fs::path assetPath = MakeAssetPath(EAssetType::Texture, assetInfo.GetGuid());
        IG_CHECK(!assetPath.empty());
        if (fs::exists(assetPath))
        {
            IG_LOG(TextureImporter, Warning, "The Asset file {} alread exist.", assetPath.string());
        }

        const HRESULT res = DirectX::SaveToDDSFile(
            targetTex.GetImages(), targetTex.GetImageCount(), targetTex.GetMetadata(),
            DirectX::DDS_FLAGS_NONE,
            assetPath.c_str());

        if (FAILED(res))
        {
            return MakeFail<Texture::Desc, ETextureImportStatus::FailedSaveAssetToFile>();
        }

        IG_CHECK(assetInfo.IsValid() && assetInfo.GetType() == EAssetType::Texture);
        return MakeSuccess<Texture::Desc, ETextureImportStatus>(assetInfo, newLoadConfig);
    }
} // namespace ig