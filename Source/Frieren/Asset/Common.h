#pragma once
#include <Core/Container.h>
#include <Core/Handle.h>
#include <Core/String.h>
#include <Core/Log.h>
#include <filesystem>
#include <fstream>

#ifndef FE_TEXT
	#define FE_TEXT(x) #x
#endif

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

		template <typename T, typename LogCategory>
		void LogVersionMismatch(const T& instance, const std::string_view infoMessage = "")
		{
			FE_LOG(LogCategory, FE_TEXT(T) " version does not match.\n\tFound: {}\nExpected: {}\nInfos: {}",
				   instance.Version, T::CurrentVersion, infoMessage);
		}
	} // namespace details

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

	/* Refer to {ResourcePath}.metadata */
	fs::path MakeResourceMetadataPath(fs::path resPath);

	/* Refer to ./Asset/{AssetType}/{GUID} */
	fs::path MakeAssetPath(const EAssetType type, const xg::Guid& guid);

	/* Refer to ./Assets/{AssetType}/{GUID}.metadata */
	fs::path MakeAssetMetadataPath(const EAssetType type, const xg::Guid& guid);

	/* Common Resource Metadata */
	struct ResourceMetadata
	{
		friend nlohmann::json& operator<<(nlohmann::json& archive, const ResourceMetadata& metadata);
		friend const nlohmann::json& operator>>(const nlohmann::json& archive, ResourceMetadata& metadata);

	public:
		[[nodiscard]]
		static inline auto MakeDefault()
		{
			return ResourceMetadata{ .Version = CurrentVersion };
		}

		[[nodiscard]]
		bool IsValidVersion() const
		{
			return Version == ResourceMetadata::CurrentVersion;
		}

	public:
		constexpr static uint64_t CurrentVersion = 1;
		uint64_t Version = 0;
		uint64_t CreationTime = 0;
		xg::Guid AssetGuid{};
		EAssetType AssetType = EAssetType::Unknown;
		bool bIsPersistent = false;
	};

	nlohmann::json& operator<<(nlohmann::json& archive, const ResourceMetadata& metadata);
	const nlohmann::json& operator>>(const nlohmann::json& archive, ResourceMetadata& metadata);

	/* Common Asset Metadata */
	struct AssetMetadata
	{
		friend nlohmann::json& operator<<(nlohmann::json& archive, const AssetMetadata& metadata);
		friend const nlohmann::json& operator>>(const nlohmann::json& archive, AssetMetadata& metadata);

	public:
		[[nodiscard]]
		static inline auto MakeDefault()
		{
			return AssetMetadata{ .Version = CurrentVersion };
		}

		[[nodiscard]]
		bool IsValidVersion() const
		{
			return Version == AssetMetadata::CurrentVersion;
		}

	public:
		constexpr static size_t CurrentVersion = 1;
		uint64_t Version = 0;
		xg::Guid Guid{};
		std::string SrcResPath{};
		EAssetType Type = EAssetType::Unknown;
		bool bIsPersistent = false;
	};

	nlohmann::json& operator<<(nlohmann::json& archive, const AssetMetadata& metadata);
	const nlohmann::json& operator>>(const nlohmann::json& archive, AssetMetadata& metadata);
} // namespace fe
