#include <Igniter.h>
#include <Core/Log.h>
#include <Core/Serializable.h>
#include <Filesystem/Utils.h>
#include <Filesystem/AsyncFileTracker.h>
#include <Asset/Utils.h>
#include <Asset/AssetMonitor.h>

IG_DEFINE_LOG_CATEGORY(AssetMonitor);

namespace ig
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
                        IG_LOG(AssetMonitor, Warning, "Asset {} ignored. {} is not valid guid.", entry.path().string(), entry.path().filename().string());
                        ++directoryItr;
                        continue;
                    }

                    fs::path metadataPath{ entry.path() };
                    metadataPath.replace_extension(details::MetadataExt);
                    if (!fs::exists(metadataPath))
                    {
                        IG_LOG(AssetMonitor, Warning, "Asset {} ignored. The metadata does not exists.", entry.path().string());
                        ++directoryItr;
                        continue;
                    }

                    const json serializedMetadata{ LoadJsonFromFile(metadataPath) };
                    AssetInfo assetInfo{};
                    serializedMetadata >> assetInfo;

                    if (!assetInfo.IsValid())
                    {
                        IG_LOG(AssetMonitor, Warning, "Asset {} ignored. The asset info is invalid.", entry.path().string());
                        ++directoryItr;
                        continue;
                    }

                    if (guidFromPath != assetInfo.Guid)
                    {
                        IG_LOG(AssetMonitor, Warning, "Asset {} ignored. The guid from filename does not match asset info guid.", entry.path().string());
                        ++directoryItr;
                        continue;
                    }

                    IG_CHECK(!virtualPathGuidTable.contains(assetInfo.VirtualPath));
                    virtualPathGuidTable.insert_or_assign(assetInfo.VirtualPath, assetInfo.Guid);
                    IG_LOG(AssetMonitor, Debug, "VirtualPath: {}, Guid: {}", assetInfo.VirtualPath.ToStringView(), assetInfo.Guid.str());
                    IG_CHECK(!guidAssetInfoTable.contains(assetInfo.Guid));
                    guidAssetInfoTable.insert_or_assign(assetInfo.Guid, assetInfo);

                    /* #sy_wip Added 된 걸로 보고 변경 정보 캐싱 */
                    // cachedAssetChangeInfo[entry.path()] = FileChangeInfo{ .Action = EFileWatchAction::Added,
                    //                                                       .FileSize = fs::file_size(entry.path()) };
                }

                ++directoryItr;
            }
        }
    }

    bool AssetMonitor::Contains(const xg::Guid guid) const
    {
        return guidAssetInfoTable.contains(guid);
    }

    bool AssetMonitor::Contains(const EAssetType assetType, const String virtualPath) const
    {
        const VirtualPathGuidTable& virtualPathGuidTable = GetVirtualPathGuidTable(assetType);
        return virtualPathGuidTable.contains(virtualPath);
    }

    xg::Guid AssetMonitor::GetGuid(const EAssetType assetType, const String virtualPath) const
    {
        const VirtualPathGuidTable& virtualPathGuidTable = GetVirtualPathGuidTable(assetType);

        const auto itr = virtualPathGuidTable.find(virtualPath);
        if (itr == virtualPathGuidTable.cend())
        {
            return xg::Guid{};
        }

        return itr->second;
    }

    std::optional<AssetInfo> AssetMonitor::GetAssetInfo(const xg::Guid guid) const
    {
        if (!guid.isValid())
        {
            return std::nullopt;
        }

        const auto itr = guidAssetInfoTable.find(guid);
        if (itr == guidAssetInfoTable.cend())
        {
            return std::nullopt;
        }

        return std::make_optional(itr->second);
    }

    std::optional<AssetInfo> AssetMonitor::GetAssetInfo(const EAssetType assetType, const String virtualPath) const
    {
        return GetAssetInfo(GetGuid(assetType, virtualPath));
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

    void AssetMonitor::Add(const AssetInfo& newInfo)
    {
        IG_CHECK(newInfo.IsValid());
        IG_CHECK(!guidAssetInfoTable.contains(newInfo.Guid));

        VirtualPathGuidTable& virtualPathGuidTable = GetVirtualPathGuidTable(newInfo.Type);
        IG_CHECK(!virtualPathGuidTable.contains(newInfo.VirtualPath));

        guidAssetInfoTable[newInfo.Guid] = newInfo;
        virtualPathGuidTable[newInfo.VirtualPath] = newInfo.Guid;
    }

    void AssetMonitor::Update(const AssetInfo& newInfo)
    {
        IG_CHECK(newInfo.IsValid());
        IG_CHECK(guidAssetInfoTable.contains(newInfo.Guid));

        const AssetInfo& oldInfo = guidAssetInfoTable[newInfo.Guid];
        IG_CHECK(newInfo.Guid == oldInfo.Guid);
        IG_CHECK(newInfo.Type == oldInfo.Type);

        VirtualPathGuidTable& virtualPathGuidTable = GetVirtualPathGuidTable(newInfo.Type);
        if (newInfo.VirtualPath != oldInfo.VirtualPath)
        {
            IG_CHECK(!virtualPathGuidTable.contains(newInfo.VirtualPath));
            virtualPathGuidTable.erase(oldInfo.VirtualPath);
            virtualPathGuidTable[newInfo.VirtualPath] = newInfo.Guid;
        }

        guidAssetInfoTable[newInfo.Guid] = newInfo;
    }

    void AssetMonitor::Remove(const AssetInfo& info)
    {
        IG_CHECK(info.IsValid());
        IG_CHECK(guidAssetInfoTable.contains(info.Guid));

        VirtualPathGuidTable& virtualPathGuidTable = GetVirtualPathGuidTable(info.Type);
        IG_CHECK(virtualPathGuidTable.contains(info.VirtualPath));
        IG_CHECK(virtualPathGuidTable[info.VirtualPath] == info.Guid);

        expiredAssetInfos[info.Guid] = info;
        virtualPathGuidTable.erase(info.VirtualPath);
        guidAssetInfoTable.erase(info.Guid);
    }

    void AssetMonitor::ReflectAllChanges()
    {
        ReflectExpiredToFiles();
        ReflectRemainedToFiles();
    }

    void AssetMonitor::ReflectExpiredToFiles()
    {
        for (const auto& expiredAssetInfo : expiredAssetInfos)
        {
            IG_CHECK(!guidAssetInfoTable.contains(expiredAssetInfo.first));
            const AssetInfo& assetInfo{ expiredAssetInfo.second };
            IG_CHECK(expiredAssetInfo.first == assetInfo.Guid);
            const fs::path metadataPath{ MakeAssetMetadataPath(assetInfo.Type, assetInfo.Guid) };
            const fs::path assetPath{ MakeAssetPath(assetInfo.Type, assetInfo.Guid) };
            if (fs::exists(metadataPath))
            {
                IG_ENSURE(fs::remove(metadataPath));
            }

            if (fs::exists(assetPath))
            {
                IG_ENSURE(fs::remove(assetPath));
            }
        }
        expiredAssetInfos.clear();
    }

    void AssetMonitor::ReflectRemainedToFiles()
    {
        IG_CHECK(expiredAssetInfos.empty());
        for (const auto& guidAssetInfo : guidAssetInfoTable)
        {
            const AssetInfo& assetInfo{ guidAssetInfo.second };
            const fs::path metadataPath{ MakeAssetMetadataPath(assetInfo.Type, assetInfo.Guid) };
            json archive{ LoadJsonFromFile(metadataPath) };
            archive << assetInfo;
            IG_ENSURE(SaveJsonToFile(metadataPath, archive));
        }
    }

} // namespace ig