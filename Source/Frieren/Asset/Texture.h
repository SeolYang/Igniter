#pragma once
#include <Asset/Common.h>
#include <D3D12/Common.h>
#include <D3D12/GpuTexture.h>
#include <Core/Handle.h>

struct ID3D11Device;

namespace fe
{
	class GpuTexture;
	class GpuView;

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

	struct TextureImportConfig : public ResourceMetadata<2>
	{
		friend nlohmann::json& operator<<(nlohmann::json& archive, const TextureImportConfig& config);
		friend const nlohmann::json& operator>>(const nlohmann::json& archive, TextureImportConfig& config);

	public:
		virtual json& Serialize(json& archive) const override;
		virtual const json& Deserialize(const json& archive) override;

	public:
		ETextureCompressionMode CompressionMode = ETextureCompressionMode::None;
		bool bGenerateMips = false;

		D3D12_FILTER Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
		D3D12_TEXTURE_ADDRESS_MODE AddressModeU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		D3D12_TEXTURE_ADDRESS_MODE AddressModeV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		D3D12_TEXTURE_ADDRESS_MODE AddressModeW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	};

	nlohmann::json& operator<<(nlohmann::json& archive, const TextureImportConfig& config);
	const nlohmann::json& operator>>(const nlohmann::json& archive, TextureImportConfig& config);

	struct TextureLoadConfig : public AssetMetadata<4>
	{
		friend nlohmann::json& operator<<(nlohmann::json& archive, const TextureLoadConfig& config);
		friend const nlohmann::json& operator>>(const nlohmann::json& archive, TextureLoadConfig& config);

	public:
		virtual json& Serialize(json& archive) const override;
		virtual const json& Deserialize(const json& archive) override;

		[[nodiscard]] bool IsArray() const { return Dimension != ETextureDimension::Tex3D && DepthOrArrayLength > 1; }

	public:
		DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
		ETextureDimension Dimension = ETextureDimension::Tex2D;
		uint32_t Width = 1;
		uint32_t Height = 1;
		uint16_t DepthOrArrayLength = 1;
		uint16_t Mips = 1;
		bool bIsCubemap = false;

		D3D12_FILTER Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
		D3D12_TEXTURE_ADDRESS_MODE AddressModeU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		D3D12_TEXTURE_ADDRESS_MODE AddressModeV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		D3D12_TEXTURE_ADDRESS_MODE AddressModeW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	};

	nlohmann::json& operator<<(nlohmann::json& archive, const TextureLoadConfig& config);
	const nlohmann::json& operator>>(const nlohmann::json& archive, TextureLoadConfig& config);

	class TextureImporter
	{
	public:
		TextureImporter();
		~TextureImporter();

		std::optional<xg::Guid> Import(const String resPathStr, std::optional<TextureImportConfig> config = std::nullopt, const bool bIsPersistent = false);

	private:
		ID3D11Device* d3d11Device = nullptr;
	};

	// 로드 후 셋업
	class RenderDevice;
	class GpuTexture;
	class GpuView;
	class GpuViewManager;
	struct Texture
	{
	public:
		TextureLoadConfig LoadConfig{};
		Handle<GpuTexture, DeferredDestroyer<GpuTexture>> TextureInstance{}; // Using Deferred Deallocation
		Handle<GpuView, GpuViewManager*> ShaderResourceView{};
		RefHandle<GpuView> TexSampler{};
	};

	class GpuUploader;
	class GpuViewManager;
	class TextureLoader
	{
	public:
		static std::optional<Texture> Load(const xg::Guid& guid, HandleManager& handleManager, RenderDevice& renderDevice, GpuUploader& gpuUploader, GpuViewManager& gpuViewManager);
	};
} // namespace fe