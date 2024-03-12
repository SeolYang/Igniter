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

	enum class EAssetType
	{
		Unknown,
		Texture,
		StaticMesh,
		SkeletalMesh,
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
	template <uint64_t CurrentVersion>
	struct ResourceMetadata
	{
	public:
		[[nodiscard]]
		bool IsLatestVersion() const
		{
			return Version == LatestVersion;
		}

		virtual json& Serialize(json& archive) const
		{
			const ResourceMetadata& metadata = *this;
			check(metadata.IsLatestVersion());
			check(metadata.CreationTime > 0);
			check(metadata.AssetGuid.isValid());
			check(metadata.AssetType != EAssetType::Unknown);

			FE_SERIALIZE_JSON(ResourceMetadata, archive, metadata, Version);
			FE_SERIALIZE_JSON(ResourceMetadata, archive, metadata, CreationTime);
			FE_SERIALIZE_GUID_JSON(ResourceMetadata, archive, metadata, AssetGuid);
			FE_SERIALIZE_ENUM_JSON(ResourceMetadata, archive, metadata, AssetType);
			FE_SERIALIZE_JSON(ResourceMetadata, archive, metadata, bIsPersistent);

			return archive;
		}

		virtual const json& Deserialize(const json& archive)
		{
			ResourceMetadata& metadata = *this;
			FE_DESERIALIZE_JSON(ResourceMetadata, archive, metadata, Version);
			FE_DESERIALIZE_JSON(ResourceMetadata, archive, metadata, CreationTime);
			FE_DESERIALIZE_GUID_JSON(ResourceMetadata, archive, metadata, AssetGuid);
			FE_DESERIALIZE_ENUM_JSON(ResourceMetadata, archive, metadata, AssetType, EAssetType::Unknown);
			FE_DESERIALIZE_JSON(ResourceMetadata, archive, metadata, bIsPersistent);

			check(metadata.IsLatestVersion());
			check(metadata.CreationTime > 0);
			check(metadata.AssetGuid.isValid());
			check(metadata.AssetType != EAssetType::Unknown);

			return archive;
		}

	public:
		using CommonMetadata = ResourceMetadata;

		constexpr static uint64_t BaseVersion = 2;
		constexpr static uint64_t LatestVersion = BaseVersion + CurrentVersion;

		uint64_t Version = 0;
		uint64_t CreationTime = 0;
		xg::Guid AssetGuid{};
		EAssetType AssetType = EAssetType::Unknown;
		bool bIsPersistent = false;
	};

	/* Common Asset Metadata */
	template <uint64_t CurrentVersion>
	struct AssetMetadata
	{
	public:
		[[nodiscard]]
		bool IsLatestVersion() const
		{
			return Version == AssetMetadata::LatestVersion;
		}

		virtual json& Serialize(json& archive) const
		{
			const AssetMetadata& metadata = *this;
			check(metadata.IsLatestVersion());
			check(metadata.Guid.isValid());
			check(!metadata.SrcResPath.empty());
			check(metadata.Type != EAssetType::Unknown);

			FE_SERIALIZE_JSON(AssetMetadata, archive, metadata, Version);
			FE_SERIALIZE_GUID_JSON(AssetMetadata, archive, metadata, Guid);
			FE_SERIALIZE_JSON(AssetMetadata, archive, metadata, SrcResPath);
			FE_SERIALIZE_ENUM_JSON(AssetMetadata, archive, metadata, Type);
			FE_SERIALIZE_JSON(AssetMetadata, archive, metadata, bIsPersistent);

			return archive;
		}

		virtual const json& Deserialize(const json& archive)
		{
			AssetMetadata& metadata = *this;
			FE_DESERIALIZE_JSON(AssetMetadata, archive, metadata, Version);
			FE_DESERIALIZE_GUID_JSON(AssetMetadata, archive, metadata, Guid);
			FE_DESERIALIZE_JSON(AssetMetadata, archive, metadata, SrcResPath);
			FE_DESERIALIZE_ENUM_JSON(AssetMetadata, archive, metadata, Type, EAssetType::Unknown);
			FE_DESERIALIZE_JSON(AssetMetadata, archive, metadata, bIsPersistent);

			check(metadata.IsLatestVersion());
			check(metadata.Guid.isValid());
			check(!metadata.SrcResPath.empty());
			check(metadata.Type != EAssetType::Unknown);
			return archive;
		}

	public:
		using CommonMetadata = AssetMetadata;

		constexpr static size_t BaseVersion = 2;
		constexpr static size_t LatestVersion = BaseVersion + CurrentVersion;

		uint64_t Version = 0;
		/* #todo Creation Time ? */
		xg::Guid Guid{};
		std::string SrcResPath{};
		EAssetType Type = EAssetType::Unknown;
		bool bIsPersistent = false;
	};

	template <typename T>
	[[nodiscard]] T MakeVersionedDefault()
	{
		T newData{};
		newData.Version = T::LatestVersion;
		return newData;
	}

	namespace details
	{
		constexpr inline std::string_view MetadataExt = ".metadata";
		constexpr inline std::string_view ResourceRootPath = "Resources";
		constexpr inline std::string_view AssetRootPath = "Assets";
		constexpr inline std::string_view TextureAssetRootPath = "Assets\\Textures";
		constexpr inline std::string_view StaticMeshAssetRootPath = "Assets\\StaticMeshes";
		constexpr inline std::string_view SkeletalMeshAssetRootPath = "Assets\\SkeletalMeshes";
		constexpr inline std::string_view ShaderAssetRootPath = "Assets\\Shaders";
		constexpr inline std::string_view AudioAssetRootPath = "Assets\\Audios";
		constexpr inline std::string_view ScriptAssetRootPath = "Assets\\Scripts";

		template <typename T, typename LogCategory>
		void LogVersionMismatch(const T& instance, const std::string_view infoMessage = "")
		{
			FE_LOG(LogCategory, FE_TEXT(T) " version does not match.\n\tFound: {}\nExpected: {}\nInfos: {}",
				   instance.Version, T::LatestVersion, infoMessage);
		}

		/* Return Versioned Default Instance, if file does not exist at provided path. */
		template <typename T, typename MismatchLogCategory = fe::LogWarn>
		T LoadMetadataFromFile(const fs::path& resMetaPath)
		{
			if (fs::exists(resMetaPath))
			{
				T metadata{};
				std::ifstream resMetaStream{ resMetaPath.c_str() };
				const json serializedResMeta{ json::parse(resMetaStream) };
				resMetaStream.close();

				serializedResMeta >> metadata;

				if (metadata.IsLatestVersion())
				{
					details::LogVersionMismatch<T, MismatchLogCategory>(metadata, resMetaPath.string());
				}

				return metadata;
			}

			return MakeVersionedDefault<T>();
		}
	} // namespace details
} // namespace fe
