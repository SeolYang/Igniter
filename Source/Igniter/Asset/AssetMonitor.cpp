#include <Igniter.h>
#include <Core/Log.h>
#include <Filesystem/Utils.h>
#include <Asset/AssetMonitor.h>

IG_DEFINE_LOG_CATEGORY(AssetMonitor);

namespace ig::details
{
    AssetMonitor::AssetMonitor()
    {
        InitVirtualPathGuidTables();
        ParseAssetDirectory();
    }

    AssetMonitor::~AssetMonitor()
    {
    }

    void AssetMonitor::InitVirtualPathGuidTables()
    {
        for (const auto assetType : magic_enum::enum_values<EAssetType>())
        {
            virtualPathGuidTables.emplace_back(assetType, VirtualPathGuidTable{});

            if (assetType != EAssetType::Unknown)
            {
                const fs::path typeDirPath{ GetAssetDirectoryPath(assetType) };
                if (!fs::exists(typeDirPath))
                {
                    fs::create_directories(typeDirPath);
                }
            }
        }
    }

    void AssetMonitor::ParseAssetDirectory()
    {
        for (const auto assetType : magic_enum::enum_values<EAssetType>())
        {
            if (assetType == EAssetType::Unknown)
            {
                continue;
            }

            VirtualPathGuidTable& virtualPathGuidTable = GetVirtualPathGuidTable(assetType);
            fs::directory_iterator directoryItr{ GetAssetDirectoryPath(assetType) };

            IG_LOG(AssetMonitor, Debug, "Root Dir: {}", GetAssetDirectoryPath(assetType).string());
            while (directoryItr != fs::end(directoryItr))
            {
                const fs::directory_entry& entry = *directoryItr;
                if (entry.is_regular_file() && !entry.path().has_extension())
                {
                    /* 최초에 온건한 형식의 메타데이터만 캐싱 됨을 보장 하여야 함! */
                    const xg::Guid guidFromPath{ entry.path().filename().string() };
                    if (!guidFromPath.isValid())
                    {
                        IG_LOG(AssetMonitor, Error, "Asset {} ignored. {} is not valid guid.",
                               entry.path().string(),
                               entry.path().filename().string());
                        ++directoryItr;
                        continue;
                    }

                    fs::path metadataPath{ entry.path() };
                    metadataPath.replace_extension(details::MetadataExt);
                    if (!fs::exists(metadataPath))
                    {
                        IG_LOG(AssetMonitor, Error, "Asset {} ignored. The metadata does not exists.", entry.path().string());
                        ++directoryItr;
                        continue;
                    }

                    json serializedMetadata{ LoadJsonFromFile(metadataPath) };
                    AssetInfo assetInfo{};
                    serializedMetadata >> assetInfo;

                    const xg::Guid guid{ assetInfo.GetGuid() };
                    const String virtualPath{ assetInfo.GetVirtualPath() };

                    if (!assetInfo.IsValid())
                    {
                        IG_LOG(AssetMonitor, Error, "Asset {} ignored. The asset info is invalid.", entry.path().string());
                        ++directoryItr;
                        continue;
                    }

                    if (guidFromPath != guid)
                    {
                        IG_LOG(AssetMonitor, Error, "{}: Asset {} ignored. The guid from filename does not match asset info guid."
                                                      " Which was {}.",
                               assetInfo.GetType(),
                               entry.path().string(),
                               guid);
                        ++directoryItr;
                        continue;
                    }

                    if (!IsValidVirtualPath(virtualPath))
                    {
                        IG_LOG(AssetMonitor, Error, "{}: Asset {} ({}) ignored. Which has invalid virtual path.",
                               assetInfo.GetType(), virtualPath, guid);
                        ++directoryItr;
                        continue;
                    }

                    if (virtualPathGuidTable.contains(virtualPath))
                    {
                        IG_LOG(AssetMonitor, Error, "{}: Asset {} ({}) ignored. Which has duplicated virtual path.",
                               assetInfo.GetType(), virtualPath, guid);
                        ++directoryItr;
                        continue;
                    }

                    if (assetInfo.GetPersistency() == EAssetPersistency::Engine)
                    {
                        IG_LOG(AssetMonitor, Warning, "{}: Found invalid asset persistency Asset {} ({}). Assumes as Default.",
                               assetInfo.GetType(), virtualPath, guid);
                        assetInfo.SetPersistency(EAssetPersistency::Default);
                        serializedMetadata << assetInfo;
                    }

                    virtualPathGuidTable[virtualPath] = guid;
                    IG_LOG(AssetMonitor, Debug, "VirtualPath: {}, Guid: {}", virtualPath, guid);
                    IG_CHECK(!Contains(guid));
                    guidSerializedDescTable[guid] = serializedMetadata;
                }

                ++directoryItr;
            }
        }
    }

    AssetMonitor::VirtualPathGuidTable& AssetMonitor::GetVirtualPathGuidTable(const EAssetType assetType)
    {
        IG_CHECK(assetType != EAssetType::Unknown);
        VirtualPathGuidTable* table = nullptr;
        for (auto& tableAssetPair : virtualPathGuidTables)
        {
            if (tableAssetPair.first == assetType)
            {
                table = &tableAssetPair.second;
                break;
            }
        }
        IG_CHECK(table != nullptr);
        return *table;
    }

    const AssetMonitor::VirtualPathGuidTable& AssetMonitor::GetVirtualPathGuidTable(const EAssetType assetType) const
    {
        IG_CHECK(assetType != EAssetType::Unknown);
        const VirtualPathGuidTable* table = nullptr;
        for (auto& tableAssetPair : virtualPathGuidTables)
        {
            if (tableAssetPair.first == assetType)
            {
                table = &tableAssetPair.second;
                break;
            }
        }
        IG_CHECK(table != nullptr);
        return *table;
    }

    bool AssetMonitor::ContainsUnsafe(const xg::Guid guid) const
    {
        return guidSerializedDescTable.contains(guid);
    }

    bool AssetMonitor::ContainsUnsafe(const EAssetType assetType, const String virtualPath) const
    {
        IG_CHECK(assetType != EAssetType::Unknown);

        const VirtualPathGuidTable& virtualPathGuidTable = GetVirtualPathGuidTable(assetType);
        const auto itr = virtualPathGuidTable.find(virtualPath);
        return itr != virtualPathGuidTable.cend() && ContainsUnsafe(itr->second);
    }

    xg::Guid AssetMonitor::GetGuidUnsafe(const EAssetType assetType, const String virtualPath) const
    {
        IG_CHECK(assetType != EAssetType::Unknown);
        IG_CHECK(IsValidVirtualPath(virtualPath));

        const VirtualPathGuidTable& virtualPathGuidTable = GetVirtualPathGuidTable(assetType);
        const auto itr = virtualPathGuidTable.find(virtualPath);
        IG_CHECK(itr != virtualPathGuidTable.cend());

        const xg::Guid guid{ itr->second };
        IG_CHECK(guid.isValid());

        return guid;
    }

    bool AssetMonitor::Contains(const xg::Guid guid) const
    {
        ReadOnlyLock lock{ mutex };
        return ContainsUnsafe(guid);
    }

    bool AssetMonitor::Contains(const EAssetType assetType, const String virtualPath) const
    {
        ReadOnlyLock lock{ mutex };
        return ContainsUnsafe(assetType, virtualPath);
    }

    xg::Guid AssetMonitor::GetGuid(const EAssetType assetType, const String virtualPath) const
    {
        ReadOnlyLock lock{ mutex };
        return GetGuidUnsafe(assetType, virtualPath);
    }

    AssetInfo AssetMonitor::GetAssetInfoUnsafe(const xg::Guid guid) const
    {
        IG_CHECK(ContainsUnsafe(guid));

        AssetInfo info{};
        const auto itr = guidSerializedDescTable.find(guid);
        itr->second >> info;

        IG_CHECK(info.IsValid());
        return info;
    }

    AssetInfo AssetMonitor::GetAssetInfo(const xg::Guid guid) const
    {
        ReadOnlyLock lock{ mutex };
        return GetAssetInfoUnsafe(guid);
    }

    AssetInfo AssetMonitor::GetAssetInfo(const EAssetType assetType, const String virtualPath) const
    {
        ReadOnlyLock lock{ mutex };
        return GetAssetInfoUnsafe(GetGuidUnsafe(assetType, virtualPath));
    }

    void AssetMonitor::UpdateInfo(const AssetInfo& newInfo)
    {
        const xg::Guid guid{ newInfo.GetGuid() };
        const String virtualPath{ newInfo.GetVirtualPath() };
        IG_CHECK(IsValidVirtualPath(virtualPath));

        ReadWriteLock rwLock{ mutex };
        IG_CHECK(newInfo.IsValid());
        IG_CHECK(ContainsUnsafe(guid));

        const AssetInfo& oldInfo = GetAssetInfoUnsafe(guid);
        const xg::Guid oldGuid{ oldInfo.GetGuid() };
        const String oldVirtualPath{ oldInfo.GetVirtualPath() };
        IG_CHECK(guid == oldGuid);
        IG_CHECK(newInfo.GetType() == oldInfo.GetType());

        VirtualPathGuidTable& virtualPathGuidTable = GetVirtualPathGuidTable(newInfo.GetType());
        if (virtualPath != oldVirtualPath)
        {
            IG_CHECK(!virtualPathGuidTable.contains(virtualPath));
            virtualPathGuidTable.erase(oldVirtualPath);
            virtualPathGuidTable[virtualPath] = guid;
        }

        guidSerializedDescTable[guid] << newInfo;
    }

    void AssetMonitor::Remove(const xg::Guid guid)
    {
        ReadWriteLock rwLock{ mutex };
        IG_CHECK(ContainsUnsafe(guid));

        const AssetInfo info = GetAssetInfoUnsafe(guid);
        IG_CHECK(info.IsValid());
        IG_CHECK(info.GetGuid() == guid);

        const String virtualPath{ info.GetVirtualPath() };

        VirtualPathGuidTable& virtualPathGuidTable = GetVirtualPathGuidTable(info.GetType());
        IG_CHECK(virtualPathGuidTable.contains(virtualPath));
        IG_CHECK(virtualPathGuidTable[virtualPath] == guid);

        expiredAssetInfos[guid] = info;
        virtualPathGuidTable.erase(virtualPath);
        guidSerializedDescTable.erase(guid);
        IG_CHECK(!ContainsUnsafe(guid));
    }

    void AssetMonitor::ReflectExpiredToFilesUnsafe()
    {
        for (const auto& expiredAssetInfoPair : expiredAssetInfos)
        {
            IG_CHECK(!ContainsUnsafe(expiredAssetInfoPair.first));
            const AssetInfo& expiredAssetInfo{ expiredAssetInfoPair.second };
            IG_CHECK(expiredAssetInfo.IsValid());
            const xg::Guid expiredGuid{ expiredAssetInfo.GetGuid() };
            const String expiredVirtualPath{ expiredAssetInfo.GetVirtualPath() };

            IG_CHECK(expiredAssetInfoPair.first == expiredGuid);
            const fs::path metadataPath{ MakeAssetMetadataPath(expiredAssetInfo.GetType(), expiredGuid) };
            const fs::path assetPath{ MakeAssetPath(expiredAssetInfo.GetType(), expiredGuid) };
            if (fs::exists(metadataPath))
            {
                IG_ENSURE(fs::remove(metadataPath));
            }

            if (fs::exists(assetPath))
            {
                IG_ENSURE(fs::remove(assetPath));
            }

            IG_LOG(AssetMonitor, Debug, "{} Asset Expired: {} ({})",
                   expiredAssetInfo.GetType(),
                   expiredVirtualPath, expiredGuid);
        }

        expiredAssetInfos.clear();
    }

    void AssetMonitor::ReflectRemainedToFilesUnsafe()
    {
        IG_CHECK(expiredAssetInfos.empty());
        for (const auto& guidSerializedMeta : guidSerializedDescTable)
        {
            const json& serializedMeta = guidSerializedMeta.second;
            AssetInfo assetInfo{};
            serializedMeta >> assetInfo;
            IG_CHECK(assetInfo.IsValid());

            if (assetInfo.GetPersistency() != EAssetPersistency::Engine)
            {
                const xg::Guid guid{ assetInfo.GetGuid() };
                IG_CHECK(guidSerializedMeta.first == guid);
                const String virtualPath{ assetInfo.GetVirtualPath() };

                const fs::path metadataPath{ MakeAssetMetadataPath(assetInfo.GetType(), guid) };
                IG_ENSURE(SaveJsonToFile(metadataPath, serializedMeta));
                IG_LOG(AssetMonitor, Debug, "{} Asset metadata Saved: {} ({})",
                       assetInfo.GetType(),
                       virtualPath, guid);
            }
        }
    }

    void AssetMonitor::SaveAllChanges()
    {
        IG_LOG(AssetMonitor, Info, "Save all info chages...");
        {
            ReadWriteLock rwLock{ mutex };
            ReflectExpiredToFilesUnsafe();
            ReflectRemainedToFilesUnsafe();
        }
        IG_LOG(AssetMonitor, Info, "All info changes saved.");
    }

    std::vector<AssetInfo> AssetMonitor::TakeSnapshots() const
    {
        ReadOnlyLock lock{ mutex };
        std::vector<AssetInfo> assetInfoSnapshots{};
        assetInfoSnapshots.reserve(guidSerializedDescTable.size());
        for (const auto& guidSerializedDesc : guidSerializedDescTable)
        {
            AssetInfo assetInfo{};
            guidSerializedDesc.second >> assetInfo;
            assetInfoSnapshots.emplace_back(assetInfo);
        }
        return assetInfoSnapshots;
    }
} // namespace ig::details