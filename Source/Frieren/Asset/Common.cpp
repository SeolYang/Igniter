#include <Asset/Common.h>

namespace fe
{
	nlohmann::json& operator<<(nlohmann::json& archive, const ResourceMetadata& metadata)
	{
		check(ResourceMetadata::CurrentVersion == metadata.Version);
		check(metadata.CreationTime > 0);
		check(metadata.AssetGuid.isValid());
		check(metadata.AssetType != EAssetType::Unknown);

		FE_SERIALIZE_JSON(ResourceMetadata, archive, metadata, Version);
		FE_SERIALIZE_JSON(ResourceMetadata, archive, metadata, CreationTime);
		FE_SERIALIZE_GUID_JSON(ResourceMetadata, archive, metadata, AssetGuid);
		FE_SERIALIZE_ENUM_JSON(ResourceMetadata, archive, metadata, AssetType);
		return archive;
	}

	ResourceMetadata& operator>>(const nlohmann::json& archive, ResourceMetadata& metadata)
	{
		FE_DESERIALIZE_JSON(ResourceMetadata, archive, metadata, Version);
		FE_DESERIALIZE_JSON(ResourceMetadata, archive, metadata, CreationTime);
		FE_DESERIALIZE_GUID_JSON(ResourceMetadata, archive, metadata, AssetGuid);
		FE_DESERIALIZE_ENUM_JSON(ResourceMetadata, archive, metadata, AssetType, EAssetType::Unknown);

		check(ResourceMetadata::CurrentVersion == metadata.Version);
		check(metadata.CreationTime > 0);
		check(metadata.AssetGuid.isValid());
		check(metadata.AssetType != EAssetType::Unknown);
		return metadata;
	}

	fs::path ToResourceMetadataPath(fs::path resPath)
	{
		resPath += details::MetadataExt;
		return resPath;
	}

	fs::path MakeAssetPath(const EAssetType type, const xg::Guid& guid)
	{
		fs::path newAssetPath{};
		if (type != EAssetType::Unknown && guid.isValid())
		{
			switch (type)
			{
				case EAssetType::Texture:
					newAssetPath = details::TextureAssetRootPath;
					break;

				case EAssetType::StaticMesh:
					newAssetPath = details::StaticMeshAssetRootPath;
					break;

				case EAssetType::Model:
					newAssetPath = details::ModelAssetRootPath;
					break;

				case EAssetType::Shader:
					newAssetPath = details::ShaderAssetRootPath;
					break;

				case EAssetType::Audio:
					newAssetPath = details::AudioAssetRootPath;
					break;

				case EAssetType::Script:
					newAssetPath = details::ScriptAssetRootPath;
					break;
			}

			newAssetPath /= guid.str();
		}
		else
		{
			checkNoEntry();
		}

		return newAssetPath;
	}

	fs::path MakeAssetMetadataPath(const EAssetType type, const xg::Guid& guid)
	{
		fs::path newAssetPath{MakeAssetPath(type, guid)};
		if (!newAssetPath.empty())
		{
			newAssetPath.replace_extension(details::MetadataExt);
		}

		return newAssetPath;
	}

} // namespace fe