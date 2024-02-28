#pragma once
#include <Asset/Common.h>
#include <Core/Handle.h>
#include <D3D12/Common.h>

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
		BC4,  /* Gray-scale */
		BC5,  /* Tangent-space normal maps */
		BC6H, /* HDR images */
		BC7	  /* High-quality color maps/Color maps + Full Alpha */
	};

	enum class ETextureDimension
	{
		Tex1D,
		Tex2D,
		Tex3D
	};

	struct TextureImportConfig
	{
		friend nlohmann::json& operator<<(nlohmann::json& archive,
										  const TextureImportConfig& config);
		friend const nlohmann::json& operator>>(const nlohmann::json& archive,
												TextureImportConfig& config);

	public:
		constexpr static size_t CurrentVersion = 2;
		size_t Version = CurrentVersion;
		ETextureCompressionMode CompressionMode = ETextureCompressionMode::None;
		bool bGenerateMips = false;

		/* Sampler */
		D3D12_FILTER Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
		D3D12_TEXTURE_ADDRESS_MODE AddressModeU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		D3D12_TEXTURE_ADDRESS_MODE AddressModeV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		D3D12_TEXTURE_ADDRESS_MODE AddressModeW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	};

	nlohmann::json& operator<<(nlohmann::json& archive,
							   const TextureImportConfig& config);
	const nlohmann::json& operator>>(const nlohmann::json& archive,
									 TextureImportConfig& config);

	struct TextureResourceMetadata
	{
		friend nlohmann::json& operator<<(nlohmann::json& archive,
										  const TextureResourceMetadata& metadata);
		friend const nlohmann::json& operator>>(const nlohmann::json& archive,
												TextureResourceMetadata& metadata);

	public:
		constexpr static size_t CurrentVersion = 1;
		size_t Version = CurrentVersion;
		ResourceMetadata Common{};
		TextureImportConfig TexImportConf{};
	};

	nlohmann::json& operator<<(nlohmann::json& archive,
							   const TextureResourceMetadata& metadata);
	const nlohmann::json& operator>>(const nlohmann::json& archive,
									 TextureResourceMetadata& metadata);

	struct TextureLoadConfig
	{
		friend nlohmann::json& operator<<(nlohmann::json& archive,
										  const TextureLoadConfig& config);
		friend const nlohmann::json& operator>>(const nlohmann::json& archive,
												TextureLoadConfig& config);

	public:
		constexpr static size_t CurrentVersion = 3;
		size_t Version = CurrentVersion;
		DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
		ETextureDimension Dimension = ETextureDimension::Tex2D;
		uint32_t Width = 1;
		uint32_t Height = 1;
		uint16_t DepthOrArrayLength = 1;
		uint16_t Mips = 1;
		bool bIsCubemap = false;

		/* Sampler */ // #todo D3D12_FILTER_TYPE 참고하여, MIN/MAG/MIP 개별 선택 후
					  // 조합
		D3D12_FILTER Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
		D3D12_TEXTURE_ADDRESS_MODE AddressModeU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		D3D12_TEXTURE_ADDRESS_MODE AddressModeV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		D3D12_TEXTURE_ADDRESS_MODE AddressModeW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	};

	nlohmann::json& operator<<(nlohmann::json& archive,
							   const TextureLoadConfig& config);
	const nlohmann::json& operator>>(const nlohmann::json& archive,
									 TextureLoadConfig& config);

	struct TextureAssetMetadata
	{
		friend nlohmann::json& operator<<(nlohmann::json& archive,
										  const TextureAssetMetadata& metadata);
		friend const nlohmann::json& operator>>(const nlohmann::json& archive,
												TextureAssetMetadata& metadata);

	public:
		constexpr static size_t CurrentVersion = 1;
		size_t Version = CurrentVersion;
		AssetMetadata Common{};
		TextureLoadConfig TexLoadConf{};
	};

	nlohmann::json& operator<<(nlohmann::json& archive,
							   const TextureAssetMetadata& metadata);
	const nlohmann::json& operator>>(const nlohmann::json& archive,
									 TextureAssetMetadata& metadata);

	class TextureImporter
	{
	public:
		static bool
		ImportTexture(const String resPathStr,
					  std::optional<TextureImportConfig> config = std::nullopt,
					  const bool bIsPersistent = false);
	};

	// 로드 후 셋업
	class Texture
	{
	public:
		TextureAssetMetadata metadata;
		Handle<dx::GpuTexture> texture;
		Handle<dx::GpuView> srv;
		RefHandle<dx::GpuView> sampler;
	};
} // namespace fe