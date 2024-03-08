#include <Asset/Texture.h>
#include <Core/Container.h>
#include <Core/Timer.h>
#include <D3D12/RenderDevice.h>
#include <D3D12/GpuView.h>
#include <D3D12/GpuTexture.h>
#include <Render/GpuViewManager.h>
#include <Engine.h>
#include <cctype>
#include <algorithm>
#include <d3d11.h>
#include <DirectXTex/DirectXTex.h>

namespace fe
{
	FE_DEFINE_LOG_CATEGORY(TextureImporterInfo, ELogVerbosity::Info)
	FE_DEFINE_LOG_CATEGORY(TextureImporterWarn, ELogVerbosity::Warning)
	FE_DEFINE_LOG_CATEGORY(TextureImporterErr, ELogVerbosity::Error)
	FE_DEFINE_LOG_CATEGORY(TextureImporterFatal, ELogVerbosity::Fatal)
	FE_DEFINE_LOG_CATEGORY(TextureLoaderInfo, ELogVerbosity::Info)
	FE_DEFINE_LOG_CATEGORY(TextureLoaderWarn, ELogVerbosity::Warning)
	FE_DEFINE_LOG_CATEGORY(TextureLoaderErr, ELogVerbosity::Error)
	FE_DEFINE_LOG_CATEGORY(TextureLoaderFatal, ELogVerbosity::Fatal)

	nlohmann::json& operator<<(nlohmann::json& archive, const TextureImportConfig& config)
	{
		check(config.Version == TextureImportConfig::CurrentVersion);

		FE_SERIALIZE_JSON(TextureImportConfig, archive, config, Version);
		FE_SERIALIZE_ENUM_JSON(TextureImportConfig, archive, config, CompressionMode);
		FE_SERIALIZE_JSON(TextureImportConfig, archive, config, bGenerateMips);
		FE_SERIALIZE_ENUM_JSON(TextureImportConfig, archive, config, Filter);
		FE_SERIALIZE_ENUM_JSON(TextureImportConfig, archive, config, AddressModeU);
		FE_SERIALIZE_ENUM_JSON(TextureImportConfig, archive, config, AddressModeV);
		FE_SERIALIZE_ENUM_JSON(TextureImportConfig, archive, config, AddressModeW);

		return archive;
	}

	const nlohmann::json& operator>>(const nlohmann::json& archive, TextureImportConfig& config)
	{
		config = {};
		FE_DESERIALIZE_JSON(TextureImportConfig, archive, config, Version);
		FE_DESERIALIZE_ENUM_JSON(TextureImportConfig, archive, config, CompressionMode,
								 ETextureCompressionMode::None);
		FE_DESERIALIZE_JSON(TextureImportConfig, archive, config, bGenerateMips);
		FE_DESERIALIZE_ENUM_JSON(TextureImportConfig, archive, config, Filter,
								 D3D12_FILTER_MIN_MAG_MIP_LINEAR);
		FE_DESERIALIZE_ENUM_JSON(TextureImportConfig, archive, config, AddressModeU,
								 D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
		FE_DESERIALIZE_ENUM_JSON(TextureImportConfig, archive, config, AddressModeV,
								 D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
		FE_DESERIALIZE_ENUM_JSON(TextureImportConfig, archive, config, AddressModeW,
								 D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

		check(config.Version == TextureImportConfig::CurrentVersion);
		return archive;
	}

	nlohmann::json& operator<<(nlohmann::json& archive, const TextureResourceMetadata& metadata)
	{
		check(metadata.Version == TextureResourceMetadata::CurrentVersion);

		FE_SERIALIZE_JSON(TextureResourceMetadata, archive, metadata, Version);
		archive << metadata.Common << metadata.ImportConfig;

		return archive;
	}

	const nlohmann::json& operator>>(const nlohmann::json& archive, TextureResourceMetadata& metadata)
	{
		metadata = {};
		FE_DESERIALIZE_JSON(TextureResourceMetadata, archive, metadata, Version);
		archive >> metadata.Common >> metadata.ImportConfig;

		check(metadata.Version == TextureResourceMetadata::CurrentVersion);
		return archive;
	}

	nlohmann::json& operator<<(nlohmann::json& archive, const TextureLoadConfig& config)
	{
		check(config.Version == TextureLoadConfig::CurrentVersion);
		check(config.Format != DXGI_FORMAT_UNKNOWN);
		check(config.Width > 0 && config.Height > 0 && config.DepthOrArrayLength > 0 && config.Mips > 0);

		FE_SERIALIZE_JSON(TextureLoadConfig, archive, config, Version);
		FE_SERIALIZE_ENUM_JSON(TextureLoadConfig, archive, config, Format);
		FE_SERIALIZE_ENUM_JSON(TextureLoadConfig, archive, config, Dimension);
		FE_SERIALIZE_JSON(TextureLoadConfig, archive, config, Width);
		FE_SERIALIZE_JSON(TextureLoadConfig, archive, config, Height);
		FE_SERIALIZE_JSON(TextureLoadConfig, archive, config, DepthOrArrayLength);
		FE_SERIALIZE_JSON(TextureLoadConfig, archive, config, Mips);
		FE_SERIALIZE_JSON(TextureLoadConfig, archive, config, bIsCubemap);
		FE_SERIALIZE_ENUM_JSON(TextureLoadConfig, archive, config, Filter);
		FE_SERIALIZE_ENUM_JSON(TextureLoadConfig, archive, config, AddressModeU);
		FE_SERIALIZE_ENUM_JSON(TextureLoadConfig, archive, config, AddressModeV);
		FE_SERIALIZE_ENUM_JSON(TextureLoadConfig, archive, config, AddressModeW);

		return archive;
	}

	const nlohmann::json& operator>>(const nlohmann::json& archive, TextureLoadConfig& config)
	{
		config = {};
		FE_DESERIALIZE_JSON(TextureLoadConfig, archive, config, Version);
		FE_DESERIALIZE_ENUM_JSON(TextureLoadConfig, archive, config, Format, DXGI_FORMAT_UNKNOWN);
		FE_DESERIALIZE_ENUM_JSON(TextureLoadConfig, archive, config, Dimension, ETextureDimension::Tex2D);
		FE_DESERIALIZE_JSON(TextureLoadConfig, archive, config, Width);
		FE_DESERIALIZE_JSON(TextureLoadConfig, archive, config, Height);
		FE_DESERIALIZE_JSON(TextureLoadConfig, archive, config, DepthOrArrayLength);
		FE_DESERIALIZE_JSON(TextureLoadConfig, archive, config, Mips);
		FE_DESERIALIZE_JSON(TextureLoadConfig, archive, config, bIsCubemap);
		FE_DESERIALIZE_ENUM_JSON(TextureLoadConfig, archive, config, Filter, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
		FE_DESERIALIZE_ENUM_JSON(TextureLoadConfig, archive, config, AddressModeU,
								 D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
		FE_DESERIALIZE_ENUM_JSON(TextureLoadConfig, archive, config, AddressModeV,
								 D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
		FE_DESERIALIZE_ENUM_JSON(TextureLoadConfig, archive, config, AddressModeW,
								 D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

		check(config.Version == TextureLoadConfig::CurrentVersion);
		check(config.Format != DXGI_FORMAT_UNKNOWN);
		check(config.Width > 0 && config.Height > 0 && config.DepthOrArrayLength > 0 && config.Mips > 0);
		return archive;
	}

	nlohmann::json& operator<<(nlohmann::json& archive, const TextureAssetMetadata& metadata)
	{
		check(metadata.Version == TextureAssetMetadata::CurrentVersion);

		FE_SERIALIZE_JSON(TextureAssetMetadata, archive, metadata, Version);
		archive << metadata.Common << metadata.LoadConfig;

		return archive;
	}

	const nlohmann::json& operator>>(const nlohmann::json& archive, TextureAssetMetadata& metadata)
	{
		metadata = {};
		FE_DESERIALIZE_JSON(TextureAssetMetadata, archive, metadata, Version);
		archive >> metadata.Common >> metadata.LoadConfig;

		check(metadata.Version == TextureAssetMetadata::CurrentVersion);
		return archive;
	}

	inline bool IsDDSExtnsion(const fs::path& extension)
	{
		std::string ext = extension.string();
		std::transform(ext.begin(), ext.end(), ext.begin(),
					   [](const char character)
					   { return static_cast<char>(::tolower(static_cast<int>(character))); });

		return ext == ".dds";
	}

	inline bool IsWICExtension(const fs::path& extension)
	{
		std::string ext = extension.string();
		std::transform(ext.begin(), ext.end(), ext.begin(),
					   [](const char character)
					   { return static_cast<char>(::tolower(static_cast<int>(character))); });

		return (ext == ".png") || (ext == ".jpg") || (ext == ".jpeg") || (ext == ".bmp") || (ext == ".gif") ||
			   (ext == ".tiff");
	}

	inline bool IsHDRExtnsion(const fs::path& extension)
	{
		std::string ext = extension.string();
		std::transform(ext.begin(), ext.end(), ext.begin(),
					   [](const char character)
					   { return static_cast<char>(::tolower(static_cast<int>(character))); });

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
			// #wip_todo 정확히 HDR 파일을 읽어왔을 때, 어떤 포맷으로 들어오는지
			// 확인 필
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
			FE_LOG(TextureImporterWarn, "Failed create d3d11 device. HRESULT: {:#X}", res);
		}
	}

	TextureImporter::~TextureImporter()
	{
		if (d3d11Device != nullptr)
		{
			d3d11Device->Release();
		}
	}

	bool TextureImporter::Import(const String resPathStr,
								 std::optional<TextureImportConfig> config /*= std::nullopt*/,
								 const bool bIsPersistent /*= false*/)
	{
		const fs::path resPath{ resPathStr.AsStringView() };
		if (!fs::exists(resPath))
		{
			FE_LOG(TextureImporterErr, "The resource does not exist at {}.", resPathStr.AsStringView());
			return false;
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
			FE_LOG(TextureImporterWarn,
				   "For DDS files, the provided configuration values are disregarded, "
				   "and the supplied file is used as the asset as it is. File: {}",
				   resPathStr.AsStringView());

			loadRes = DirectX::LoadFromDDSFile(
				resPath.c_str(),
				DirectX::DDS_FLAGS_NONE,
				&texMetadata, targetTex);
			bIsDDSFormat = true;
		}
		else if (IsWICExtension(resExtension))
		{
			loadRes = DirectX::LoadFromWICFile(
				resPath.c_str(),
				DirectX::WIC_FLAGS_FORCE_LINEAR,
				&texMetadata, targetTex);
		}
		else if (IsHDRExtnsion(resExtension))
		{
			loadRes = DirectX::LoadFromHDRFile(resPath.c_str(), &texMetadata, targetTex);
			bIsHDRFormat = true;
		}
		else
		{
			FE_LOG(TextureImporterErr, "Found not supported texture extension from \"{}\".",
				   resPathStr.AsStringView());
			return false;
		}

		if (FAILED(loadRes))
		{
			FE_LOG(TextureImporterErr, "Failed to load texture from {}.", resPathStr.AsStringView());
			return false;
		}

		/* Get Texture Import Config from metadata file, make default if file does not exists. */
		bool bPersistencyRequired = bIsPersistent;
		const fs::path resMetadataPath{ MakeResourceMetadataPath(resPath) };
		if (!config)
		{
			if (fs::exists(resMetadataPath))
			{
				std::ifstream resMetadataStream{ resMetadataPath.c_str() };
				const json serializedResMetadata{ json::parse(resMetadataStream) };
				resMetadataStream.close();

				TextureResourceMetadata texResMetadata{};
				serializedResMetadata >> texResMetadata;

				if (!texResMetadata.ImportConfig.IsValidVersion())
				{
					details::LogVersionError<TextureImportConfig, TextureImporterWarn>(
						texResMetadata.ImportConfig,
						resMetadataPath.string());
				}

				bPersistencyRequired |= texResMetadata.Common.bIsPersistent;
				config = texResMetadata.ImportConfig;
			}
			else
			{
				config = TextureImportConfig::MakeDefault();
			}
		}

		if (!bIsDDSFormat)
		{
			check(texMetadata.width > 0 && texMetadata.height > 0 && texMetadata.depth > 0 &&
				  texMetadata.arraySize > 0);
			check(!texMetadata.IsVolumemap() || (texMetadata.IsVolumemap() && texMetadata.arraySize == 1));
			check(texMetadata.format != DXGI_FORMAT_UNKNOWN);
			check(!DirectX::IsSRGB(texMetadata.format));

			/* Generate full mipmap chain. */
			if (config->bGenerateMips)
			{
				DirectX::ScratchImage mipChain{};
				const HRESULT genRes = DirectX::GenerateMipMaps(
					targetTex.GetImages(), targetTex.GetImageCount(), targetTex.GetMetadata(),
					bIsHDRFormat ? DirectX::TEX_FILTER_FANT : DirectX::TEX_FILTER_LINEAR, 0, mipChain);
				if (FAILED(genRes))
				{
					FE_LOG(TextureImporterErr, "Failed to generate mipchain. HRESULT: {:#X}, File: {}",
						   genRes, resPathStr.AsStringView());

					return false;
				}

				texMetadata = mipChain.GetMetadata();
				targetTex = std::move(mipChain);
			}

			/* Compress Texture*/
			if (config->CompressionMode != ETextureCompressionMode::None)
			{
				if (bIsHDRFormat && config->CompressionMode != ETextureCompressionMode::BC6H)
				{
					FE_LOG(TextureImporterWarn,
						   "The compression method for HDR extension textures is "
						   "enforced "
						   "to be the 'BC6H' algorithm. File: {}",
						   resPathStr.AsStringView());

					config->CompressionMode = ETextureCompressionMode::BC6H;
				}
				else if (IsGreyScaleFormat(texMetadata.format) &&
						 config->CompressionMode == ETextureCompressionMode::BC4)
				{
					FE_LOG(TextureImporterWarn,
						   "The compression method for greyscale-formatted "
						   "textures is "
						   "enforced to be the 'BC4' algorithm. File: {}",
						   resPathStr.AsStringView());

					config->CompressionMode = ETextureCompressionMode::BC4;
				}

				auto compFlags = static_cast<unsigned long>(DirectX::TEX_COMPRESS_PARALLEL);
				const DXGI_FORMAT compFormat = AsBCnFormat(config->CompressionMode, texMetadata.format);

				const bool bIsGPUCodecAvailable = d3d11Device != nullptr &&
												  (config->CompressionMode == ETextureCompressionMode::BC6H ||
												   config->CompressionMode == ETextureCompressionMode::BC7);

				HRESULT compRes = S_FALSE;
				DirectX::ScratchImage compTex{};
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

				if (FAILED(compRes))
				{
					if (bIsGPUCodecAvailable)
					{
						FE_LOG(TextureImporterErr, "Failed to compress with GPU Codec.");
					}

					FE_LOG(TextureImporterErr,
						   "Failed to compress using {}. From {} to {}. HRESULT: {:#X}, "
						   "File: {}",
						   magic_enum::enum_name(config->CompressionMode),
						   magic_enum::enum_name(texMetadata.format), magic_enum::enum_name(compFormat),
						   compRes,
						   resPathStr.AsStringView());

					return false;
				}

				texMetadata = compTex.GetMetadata();
				targetTex = std::move(compTex);
			}
		}

		/*************************** Configures output ***************************/
		const size_t creationTime = Timer::Now();
		const xg::Guid newGuid = xg::newGuid();

		/* Configure Texture Resource Metadata */
		config->Version = TextureImportConfig::CurrentVersion;
		const TextureResourceMetadata newResMetadata{
			.Version = TextureResourceMetadata::CurrentVersion,
			.Common = {
				.Version = ResourceMetadata::CurrentVersion,
				.CreationTime = creationTime,
				.AssetGuid = newGuid,
				.AssetType = EAssetType::Texture,
				.bIsPersistent = bPersistencyRequired },

			.ImportConfig = *config
		};

		/* Serialize Texture Resource Metadata & Save To File */
		{
			json serializedJson{};
			serializedJson << newResMetadata;

			std::ofstream fileStream{ resMetadataPath.c_str() };
			fileStream << serializedJson.dump(4);
		}

		/* Configure Texture Asset Metadata */
		texMetadata = targetTex.GetMetadata();
		const auto convertDxTexDim = [](const DirectX::TEX_DIMENSION dim)
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
		};

		const ETextureDimension texDim = convertDxTexDim(texMetadata.dimension);
		const TextureAssetMetadata newAssetMetadata{
			.Version = TextureAssetMetadata::CurrentVersion,
			.Common = {
				.Version = AssetMetadata::CurrentVersion,
				.Guid = newGuid,
				.SrcResPath = resPathStr.AsString(),
				.Type = EAssetType::Texture,
				.bIsPersistent = bPersistencyRequired },
			.LoadConfig = { .Version = TextureLoadConfig::CurrentVersion, .Format = texMetadata.format, .Dimension = texDim, .Width = static_cast<uint32_t>(texMetadata.width), .Height = static_cast<uint32_t>(texMetadata.height), .DepthOrArrayLength = static_cast<uint16_t>(texMetadata.IsVolumemap() ? texMetadata.depth : texMetadata.arraySize), .Mips = static_cast<uint16_t>(texMetadata.mipLevels), .bIsCubemap = texMetadata.IsCubemap(), .Filter = config->Filter, .AddressModeU = config->AddressModeU, .AddressModeV = config->AddressModeV, .AddressModeW = config->AddressModeW }
		};

		/* Create Texture Asset root directory if does not exist */
		{
			const fs::path texAssetDirPath = details::TextureAssetRootPath;
			if (!fs::exists(texAssetDirPath))
			{
				if (!fs::create_directories(texAssetDirPath))
				{
					FE_LOG(TextureImporterWarn, "Failed to create texture asset root {}.", texAssetDirPath.string());
				}
			}
		}

		/* Serialize Texture Asset Metadata & Save To File */
		{
			json serializedJson{};
			serializedJson << newAssetMetadata;

			const fs::path assetMetadataPath = MakeAssetMetadataPath(EAssetType::Texture, newGuid);
			std::ofstream fileStream{ assetMetadataPath.c_str() };
			if (!fileStream.is_open())
			{
				FE_LOG(TextureImporterErr, "Failed to open asset metadata file {}.", assetMetadataPath.string());
				return false;
			}

			fileStream << serializedJson.dump(4);
		}

		/* Save data to asset file */
		{
			const fs::path assetPath = MakeAssetPath(EAssetType::Texture, newGuid);
			if (fs::exists(assetPath))
			{
				FE_LOG(TextureImporterWarn, "The Asset file {} alread exist.", assetPath.string());
			}

			const HRESULT res = DirectX::SaveToDDSFile(
				targetTex.GetImages(), targetTex.GetImageCount(), targetTex.GetMetadata(),
				DirectX::DDS_FLAGS_NONE,
				assetPath.c_str());

			if (FAILED(res))
			{
				FE_LOG(TextureImporterErr, "Failed to save texture asset file {}. HRESULT: {:#X}", assetPath.string(), res);
				return false;
			}
		}

		return true;
	}

	std::optional<Texture> TextureLoader::Load(const xg::Guid& guid, HandleManager&, RenderDevice& renderDevice, GpuUploader&)
	{
		const fs::path assetMetaPath = MakeAssetMetadataPath(EAssetType::Texture, guid);

		if (fs::exists(assetMetaPath))
		{
			FE_LOG(TextureLoaderErr, "Texture Asset Metadata \"{}\" does not exists.", assetMetaPath.string());
			return std::nullopt;
		}

		TextureAssetMetadata assetMetadata{};
		std::ifstream assetMetaStream{ assetMetaPath.c_str() };
		const json serializedAssetMeta{ json::parse(assetMetaStream) };
		assetMetaStream.close();

		serializedAssetMeta >> assetMetadata;

		/* Check Asset Metadata */
		if (!assetMetadata.IsValidVersion())
		{
			details::LogVersionError<TextureAssetMetadata, TextureLoaderWarn>(assetMetadata, assetMetaPath.string());
			details::LogVersionError<AssetMetadata, TextureLoaderWarn>(assetMetadata.Common, assetMetaPath.string());
			details::LogVersionError<TextureLoadConfig, TextureLoaderWarn>(assetMetadata.LoadConfig, assetMetaPath.string());
		}

		const AssetMetadata& assetCommon = assetMetadata.Common;
		if (assetCommon.Guid != guid)
		{
			FE_LOG(TextureLoaderErr, "Asset guid does not match. Expected: {}, Found: {}",
				   guid.str(), assetCommon.Guid.str());
			return std::nullopt;
		}

		if (assetCommon.Type != EAssetType::Texture)
		{
			FE_LOG(TextureLoaderErr, "Asset type does not match. Expected: {}, Found: {}",
				   magic_enum::enum_name(EAssetType::Texture),
				   magic_enum::enum_name(assetCommon.Type));

			return std::nullopt;
		}

		/* Check TextureLoadConfig data */
		const TextureLoadConfig& loadConfig = assetMetadata.LoadConfig;
		if (loadConfig.Width == 0 || loadConfig.Height == 0 || loadConfig.DepthOrArrayLength == 0 || loadConfig.Mips == 0)
		{
			FE_LOG(TextureLoaderErr, "Load Config has invalid values. Width: {}, Height: {}, DepthOrArrayLength: {}, Mips: {}",
				   loadConfig.Width, loadConfig.Height, loadConfig.DepthOrArrayLength, loadConfig.Mips);
			return std::nullopt;
		}

		if (loadConfig.Format == DXGI_FORMAT_UNKNOWN)
		{
			FE_LOG(TextureLoaderErr, "Load Config format is unknown.");
			return std::nullopt;
		}

		const fs::path assetPath = MakeAssetPath(EAssetType::Texture, guid);
		if (fs::exists(assetPath))
		{
			FE_LOG(TextureLoaderErr, "Texture Asset \"{}\" does not exist.", assetPath.string());
			return std::nullopt;
		}

		// #wip_todo Texture Load 구현
		// read asset file
		//DirectX::LoadFromDDSFile(assetPath.c_str(), )
		// make GpuTexture
		GPUTextureDesc texDesc{};
		// setup ex desc
		std::optional<GpuTexture> newTex = renderDevice.CreateTexture(texDesc);
		if (!newTex)
		{
			FE_LOG(TextureLoaderErr, "Failed to create GpuTexture from render device.");
			return std::nullopt;
		}
		// upload data to GpuTexture -> initial Layout == COMMON
		// make srv for texture
		// make sampler
		// make texture object & return

		unimplemented();
		return std::nullopt;
	}
} // namespace fe

#include <iostream>
namespace fe::details
{
	struct DirectXTexInitializer
	{
	public:
		DirectXTexInitializer()
		{
			const HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
			if (FAILED(hr))
			{
				std::cerr << "Failed to initialize DirectXTex. HRESULT: "
						  << std::hex << hr << std::endl;
			}
		}
	};

#pragma warning(push)
#pragma warning(disable : 4074)
	thread_local DirectXTexInitializer dxTexInitializer;
#pragma warning(pop)
} // namespace fe::details
