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
        CleanupOrphanFiles();
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

            IG_LOG(AssetMonitor, Debug, "* Parsing {} type root dir ({})...", assetType, GetAssetDirectoryPath(assetType).string());
            while (directoryItr != fs::end(directoryItr))
            {
                const fs::directory_entry& entry = *directoryItr;
                if (entry.is_regular_file() && !entry.path().has_extension())
                {
                    /* 최초에 온건한 형식의 메타데이터만 캐싱 됨을 보장 하여야 함! */
                    const Guid guidFromPath{ entry.path().filename().string() };
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

                    const Guid guid{ assetInfo.GetGuid() };
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

                    if (virtualPathGuidTable.contains(virtualPath))
                    {
                        IG_LOG(AssetMonitor, Error, "{}: Asset {} ({}) ignored. Which has duplicated virtual path.",
                               assetInfo.GetType(), virtualPath, guid);
                        ++directoryItr;
                        continue;
                    }

                    if (assetInfo.GetScope() == EAssetScope::Engine)
                    {
                        IG_LOG(AssetMonitor, Warning, "{}: Found invalid asset scope Asset {} ({}). Assumes as Managed.",
                               assetInfo.GetType(), virtualPath, guid);
                        assetInfo.SetScope(EAssetScope::Managed);
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

    bool AssetMonitor::ContainsUnsafe(const Guid guid) const
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

    Guid AssetMonitor::GetGuidUnsafe(const EAssetType assetType, const String virtualPath) const
    {
        IG_CHECK(assetType != EAssetType::Unknown);
        IG_CHECK(IsValidVirtualPath(virtualPath));

        const VirtualPathGuidTable& virtualPathGuidTable = GetVirtualPathGuidTable(assetType);
        const auto itr = virtualPathGuidTable.find(virtualPath);
        IG_CHECK(itr != virtualPathGuidTable.cend());

        const Guid guid{ itr->second };
        IG_CHECK(guid.isValid());

        return guid;
    }

    bool AssetMonitor::Contains(const Guid guid) const
    {
        ReadOnlyLock lock{ mutex };
        return ContainsUnsafe(guid);
    }

    bool AssetMonitor::Contains(const EAssetType assetType, const String virtualPath) const
    {
        ReadOnlyLock lock{ mutex };
        return ContainsUnsafe(assetType, virtualPath);
    }

    Guid AssetMonitor::GetGuid(const EAssetType assetType, const String virtualPath) const
    {
        ReadOnlyLock lock{ mutex };
        return GetGuidUnsafe(assetType, virtualPath);
    }

    AssetInfo AssetMonitor::GetAssetInfoUnsafe(const Guid guid) const
    {
        IG_CHECK(ContainsUnsafe(guid));

        AssetInfo info{};
        const auto itr = guidSerializedDescTable.find(guid);
        itr->second >> info;

        IG_CHECK(info.IsValid());
        return info;
    }

    AssetInfo AssetMonitor::GetAssetInfo(const Guid guid) const
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
        const Guid guid{ newInfo.GetGuid() };
        const String virtualPath{ newInfo.GetVirtualPath() };
        IG_CHECK(IsValidVirtualPath(virtualPath));

        ReadWriteLock rwLock{ mutex };
        IG_CHECK(newInfo.IsValid());
        IG_CHECK(ContainsUnsafe(guid));

        const AssetInfo& oldInfo = GetAssetInfoUnsafe(guid);
        const Guid oldGuid{ oldInfo.GetGuid() };
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

    void AssetMonitor::Remove(const Guid guid, const bool bShouldExpired)
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

        if (bShouldExpired)
        {
            expiredAssetInfos[guid] = info;
        }

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
            const Guid expiredGuid{ expiredAssetInfo.GetGuid() };
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

            if (assetInfo.GetScope() != EAssetScope::Engine)
            {
                const Guid guid{ assetInfo.GetGuid() };
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

    void AssetMonitor::CleanupOrphanFiles()
    {
        std::vector<fs::path> orphanFiles{};
        for (const auto assetType : magic_enum::enum_values<EAssetType>())
        {
            if (assetType == EAssetType::Unknown)
            {
                continue;
            }

            fs::directory_iterator directoryItr{ GetAssetDirectoryPath(assetType) };
            while (directoryItr != fs::end(directoryItr))
            {
                fs::path path{ directoryItr->path() };
                if (!fs::is_regular_file(path))
                {
                    ++directoryItr;
                    continue;
                }

                Guid guid{ path.filename().replace_extension().string() };
                if (!guid.isValid())
                {
                    ++directoryItr;
                    continue;
                }

                const bool bIsOrphanMetadata{ path.has_extension() && !fs::exists(path.replace_extension()) };
                const bool bIsOrphanAssetFile{ !path.has_extension() && !fs::exists(path.replace_extension(details::MetadataExt)) };
                if (bIsOrphanMetadata || bIsOrphanAssetFile)
                {
                    orphanFiles.emplace_back(directoryItr->path());
                }

                ++directoryItr;
            }
        }

        for (const fs::path& orphanFilePath : orphanFiles)
        {
            IG_LOG(AssetMonitor, Info, "Removing unreferenced(orphan) file: {}...", orphanFilePath.string());
            fs::remove(orphanFilePath);
        }
    }

    void AssetMonitor::SaveAllChanges()
    {
        IG_LOG(AssetMonitor, Info, "Save all info chages...");
        {
            ReadWriteLock rwLock{ mutex };
            ReflectExpiredToFilesUnsafe();
            ReflectRemainedToFilesUnsafe();
            CleanupOrphanFiles();
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