#include <Asset/Texture.h>
#include <Core/Log.h>
#include <Engine.h>
#include <DirectXTex/DirectXTex.h>

namespace fe
{
	FE_DEFINE_LOG_CATEGORY(TextureImporterWarn, ELogVerbosiy::Warning)

	nlohmann::json& operator<<(nlohmann::json& archive, const TextureImportConfig& config)
	{
		check(config.Version == TextureImportConfig::CurrentVersion);

		FE_SERIALIZE_JSON(TextureImportConfig, archive, config, Version);
		FE_SERIALIZE_ENUM_JSON(TextureImportConfig, archive, config, CompressionMode);
		FE_SERIALIZE_JSON(TextureImportConfig, archive, config, bGenerateMips);

		return archive;
	}

	const nlohmann::json& operator>>(const nlohmann::json& archive, TextureImportConfig& config)
	{
		config = {};
		FE_DESERIALIZE_JSON(TextureImportConfig, archive, config, Version);
		FE_DESERIALIZE_ENUM_JSON(TextureImportConfig, archive, config, CompressionMode, ETextureCompressionMode::None);
		FE_DESERIALIZE_JSON(TextureImportConfig, archive, config, bGenerateMips);

		check(config.Version == TextureImportConfig::CurrentVersion);
		return archive;
	}

	nlohmann::json& operator<<(nlohmann::json& archive, const TextureResourceMetadata& metadata)
	{
		check(metadata.Version == TextureResourceMetadata::CurrentVersion);

		FE_SERIALIZE_JSON(TextureResourceMetadata, archive, metadata, Version);
		archive << metadata.Common << metadata.TexImportConf;

		return archive;
	}

	const nlohmann::json& operator>>(const nlohmann::json& archive, TextureResourceMetadata& metadata)
	{
		metadata = {};
		FE_DESERIALIZE_JSON(TextureResourceMetadata, archive, metadata, Version);
		archive >> metadata.Common >> metadata.TexImportConf;

		check(metadata.Version == TextureResourceMetadata::CurrentVersion);
		return archive;
	}

	nlohmann::json& operator<<(nlohmann::json& archive, const TextureLoadConfig& config)
	{
		check(config.Version == TextureLoadConfig::CurrentVersion);
		check(config.Format != DXGI_FORMAT_UNKNOWN);
		check(config.Width > 0 && config.Height > 0 && config.DepthOrArrayLength > 1 && config.Mips > 1);

		FE_SERIALIZE_JSON(TextureLoadConfig, archive, config, Version);
		FE_SERIALIZE_ENUM_JSON(TextureLoadConfig, archive, config, Format);
		FE_SERIALIZE_JSON(TextureLoadConfig, archive, config, Width);
		FE_SERIALIZE_JSON(TextureLoadConfig, archive, config, Height);
		FE_SERIALIZE_JSON(TextureLoadConfig, archive, config, DepthOrArrayLength);
		FE_SERIALIZE_JSON(TextureLoadConfig, archive, config, Mips);
		FE_SERIALIZE_JSON(TextureLoadConfig, archive, config, bIsArray);
		FE_SERIALIZE_JSON(TextureLoadConfig, archive, config, bIsCubemap);

		return archive;
	}

	const nlohmann::json& operator>>(const nlohmann::json& archive, TextureLoadConfig& config)
	{
		config = {};
		FE_DESERIALIZE_JSON(TextureLoadConfig, archive, config, Version);
		FE_DESERIALIZE_ENUM_JSON(TextureLoadConfig, archive, config, Format, DXGI_FORMAT_UNKNOWN);
		FE_DESERIALIZE_JSON(TextureLoadConfig, archive, config, Width);
		FE_DESERIALIZE_JSON(TextureLoadConfig, archive, config, Height);
		FE_DESERIALIZE_JSON(TextureLoadConfig, archive, config, DepthOrArrayLength);
		FE_DESERIALIZE_JSON(TextureLoadConfig, archive, config, Mips);
		FE_DESERIALIZE_JSON(TextureLoadConfig, archive, config, bIsArray);
		FE_DESERIALIZE_JSON(TextureLoadConfig, archive, config, bIsCubemap);

		check(config.Version == TextureLoadConfig::CurrentVersion);
		check(config.Format != DXGI_FORMAT_UNKNOWN);
		check(config.Width > 0 && config.Height > 0 && config.DepthOrArrayLength > 1 && config.Mips > 1);
		return archive;
	}

	nlohmann::json& operator<<(nlohmann::json& archive, const TextureAssetMetadata& metadata)
	{
		check(metadata.Version == TextureAssetMetadata::CurrentVersion);

		FE_SERIALIZE_JSON(TextureAssetMetadata, archive, metadata, Version);
		archive << metadata.Common << metadata.TexLoadConf;

		return archive;
	}

	const nlohmann::json& operator>>(const nlohmann::json& archive, TextureAssetMetadata& metadata)
	{
		metadata = {};
		FE_DESERIALIZE_JSON(TextureAssetMetadata, archive, metadata, Version);
		archive >> metadata.Common >> metadata.TexLoadConf;

		check(metadata.Version == TextureAssetMetadata::CurrentVersion);
		return archive;
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
				std::cerr << "Failed to initialize DirectXTex. HRESULT: " << std::hex << hr << std::endl;
			}
		}
	};

#pragma warning(push)
#pragma warning(disable : 4074)
	thread_local DirectXTexInitializer dxTexInitializer;
#pragma warning(pop)
} // namespace fe::details
