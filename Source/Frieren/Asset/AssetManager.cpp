#include <Asset/AssetManager.h>
#include <Core/Assert.h>
#include <Core/Log.h>
#include <Engine.h>
#include <fstream>

namespace fe
{
	FE_DEFINE_LOG_CATEGORY(AssetManagerInfo, ELogVerbosiy::Info)
	FE_DEFINE_LOG_CATEGORY(AssetManagerWarn, ELogVerbosiy::Warning)
	FE_DEFINE_LOG_CATEGORY(AssetManagerErr, ELogVerbosiy::Error)
	FE_DEFINE_LOG_CATEGORY(AssetManagerDbg, ELogVerbosiy::Debug)
	FE_DEFINE_LOG_CATEGORY(AssetManagerFatal, ELogVerbosiy::Fatal)

	std::optional<xg::Guid> AssetManager::QueryGuid(const String resPathStr) const
	{
		const auto itr = resPathGuidTable.find(resPathStr);
		if (itr != resPathGuidTable.cend())
		{
			return itr->second;
		}

		fs::path resPath = fs::path{ resPathStr.AsStringView() };
		if (!fs::exists(resPath))
		{
			FE_LOG(AssetManagerWarn, "Does not found resource {}.", resPath.string());
			return std::nullopt;
		}

		const fs::path resMetadataPath{ ToResourceMetadataPath(std::move(resPath)) };
		if (fs::exists(resMetadataPath))
		{
			std::ifstream	 jsonFileStream{ resMetadataPath };
			nlohmann::json	 serializedResMetadata = nlohmann::json::parse(jsonFileStream);
			ResourceMetadata resMetadata;
			serializedResMetadata >> resMetadata;
			if (resMetadata.AssetGuid.isValid())
			{
				return resMetadata.AssetGuid;
			}

			FE_LOG(AssetManagerWarn, "Asset GUID from metadata({}) is not valid. {}", resMetadataPath.string(), resMetadata.AssetGuid.str());
		}
		else
		{
			FE_LOG(AssetManagerWarn, "Does not found resource metadata {}.", resMetadataPath.string());
		}

		return std::nullopt;
	}

} // namespace fe