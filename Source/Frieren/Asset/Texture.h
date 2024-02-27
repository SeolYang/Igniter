#pragma once
#include <Asset/Common.h>
#include <Core/Handle.h>

namespace fe::dx
{
	class GpuTexture;
	class GpuView;
} // namespace fe::dx

namespace fe
{
	enum class ETextureCompressionMode
	{
		None,
		BC3,  /* Color Maps + Alpha */
		BC4,  /* Gray-scale */
		BC5,  /* Tangent-space normal maps */
		BC6H, /* HDR images */
		BC7	  /* High-quality color maps/Color maps + Full Alpha */
	};

	struct TextureImportConfig
	{
		friend nlohmann::json&		 operator<<(nlohmann::json& archive, const TextureImportConfig& config);
		friend const nlohmann::json& operator>>(const nlohmann::json& archive, TextureImportConfig& config);

	public:
		constexpr static size_t CurrentVersion = 1;
		size_t					Version = 0;
		ETextureCompressionMode CompressionMode = ETextureCompressionMode::None;
		/* 원본 이미지가 BCn 포맷이 아니며 밉맵 체인을 포함 하지 않는 경우에만 유효. */
		bool bGenerateMips = false;
	};

	nlohmann::json&		  operator<<(nlohmann::json& archive, const TextureImportConfig& config);
	const nlohmann::json& operator>>(const nlohmann::json& archive, TextureImportConfig& config);

	struct TextureResourceMetadata
	{
		friend nlohmann::json&		 operator<<(nlohmann::json& archive, const TextureResourceMetadata& metadata);
		friend const nlohmann::json& operator>>(const nlohmann::json& archive, TextureResourceMetadata& metadata);

	public:
		constexpr static size_t CurrentVersion = 1;
		size_t					Version = 0;
		ResourceMetadata		Common{};
		TextureImportConfig		TexImportConf{};
	};

	nlohmann::json&		  operator<<(nlohmann::json& archive, const TextureResourceMetadata& metadata);
	const nlohmann::json& operator>>(const nlohmann::json& archive, TextureResourceMetadata& metadata);

	struct TextureLoadConfig
	{
		friend nlohmann::json&		 operator<<(nlohmann::json& archive, const TextureLoadConfig& config);
		friend const nlohmann::json& operator>>(const nlohmann::json& archive, TextureLoadConfig& config);

	public:
		constexpr static size_t CurrentVersion = 1;
		size_t					Version = CurrentVersion;
		DXGI_FORMAT				Format = DXGI_FORMAT_UNKNOWN;
		uint32_t				Width = 1;
		uint32_t				Height = 1;
		uint32_t				DepthOrArrayLength = 1;
		uint32_t				Mips = 1;
		bool					bIsArray = false;
		bool					bIsCubemap = false;
	};

	nlohmann::json&		  operator<<(nlohmann::json& archive, const TextureLoadConfig& config);
	const nlohmann::json& operator>>(const nlohmann::json& archive, TextureLoadConfig& config);

	struct TextureAssetMetadata
	{
		friend nlohmann::json&		 operator<<(nlohmann::json& archive, const TextureAssetMetadata& metadata);
		friend const nlohmann::json& operator>>(const nlohmann::json& archive, TextureAssetMetadata& metadata);

	public:
		constexpr static size_t CurrentVersion = 1;
		size_t					Version = 0;
		AssetMetadata			Common{};
		TextureLoadConfig		TexLoadConf{};
	};

	nlohmann::json&		  operator<<(nlohmann::json& archive, const TextureAssetMetadata& metadata);
	const nlohmann::json& operator>>(const nlohmann::json& archive, TextureAssetMetadata& metadata);

	class TextureImporter
	{
	public:
		static bool ImportTexture(const String resPath, std::optional<TextureImportConfig> config = std::nullopt);
	};

	// 로드 후 셋업
	class Texture
	{
	public:
		TextureAssetMetadata   metadata;
		Handle<dx::GpuTexture> texture;
		Handle<dx::GpuView>	   srv;
		RefHandle<dx::GpuView> sampler;
	};
} // namespace fe