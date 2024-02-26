#pragma once
#include <Core/Handle.h>
#include <Core/Container.h>
#include <Core/String.h>
#include <filesystem>

namespace fe
{
	namespace fs = std::filesystem;

	namespace details
	{
		constexpr inline std::string_view MetadataExt = ".metadata";
		constexpr inline std::string_view ResourceRootPath = "Resources";
		constexpr inline std::string_view AssetRootPath = "Assets";
		constexpr inline std::string_view TextureAssetRootPath = "Assets\\Textures";
		constexpr inline std::string_view StaticMeshAssetRootPath = "Assets\\StaticMeshes";
		constexpr inline std::string_view ModelAssetRootPath = "Assets\\Models";
		constexpr inline std::string_view ShaderAssetRootPath = "Assets\\Shaders";
		constexpr inline std::string_view AudioAssetRootPath = "Assets\\Audios";
		constexpr inline std::string_view ScriptAssetRootPath = "Assets\\Scripts";
	}

	enum class EAssetType
	{
		Unknown,
		Texture,
		StaticMesh,
		Model,
		Shader,
		Audio,
		Script,
		// Scene
	};

	struct ResourceMetadata
	{
		friend nlohmann::json&	 operator<<(nlohmann::json& archive, const ResourceMetadata& metadata);
		friend ResourceMetadata& operator>>(const nlohmann::json& archive, ResourceMetadata& metadata);

	public:
		constexpr static uint64_t CurrentVersion = 1;
		uint64_t				  Version = CurrentVersion;
		uint64_t				  CreationTime = 0;
		xg::Guid				  AssetGuid{};
		EAssetType				  AssetType = EAssetType::Unknown;
	};

	nlohmann::json&	  operator<<(nlohmann::json& archive, const ResourceMetadata& metadata);
	ResourceMetadata& operator>>(const nlohmann::json& archive, ResourceMetadata& metadata);

	bool IsSupportedTextureResource(const fs::path& textureResPath);
	bool IsSupportedStaticMeshResource(const fs::path& staticMeshResPath);
	// bool IsSupportedModelResource(const fs::path& modelResPath);
	// bool IsSupportedShaderResource(const fs::path& shaderResPath);
	// bool IsSupportedAudioResource(const fs::path& audioResPath);
	// bool IsSupportedScriptResource(const fs::path& scriptResPath);

	/* Refer to {ResourcePath}.metadata */
	fs::path ToResourceMetadataPath(fs::path resPath);

	/* Refer to ./Asset/{AssetType}/{GUID} */
	fs::path MakeAssetPath(const EAssetType type, const xg::Guid& guid);

	/* Refer to ./Assets/{AssetType}/{GUID}.metadata */
	fs::path MakeAssetMetadataPath(const EAssetType type, const xg::Guid& guid);
} // namespace fe
