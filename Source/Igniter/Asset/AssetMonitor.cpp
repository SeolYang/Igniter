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
    AssetMonitor::AssetMonitor() : tracker(std::make_unique<AsyncFileTracker>())
    {
        expiredAssetInfos.reserve(ReservedExpiredAssetInfoBufferSize);
        InitVirtualPathGuidTables();
        ParseAssetDirectory();
        StartTracking();
    }

    AssetMonitor::~AssetMonitor()
    {
    }

    void AssetMonitor::ProcessBufferedNotifications()
    {
        ReadWriteLock rwLock{ bufferMutex };

        bool bBufferProcessingPostponed = false;
        for (size_t idx = 0; idx < buffer.size(); ++idx)
        {
            const auto& notification = buffer[idx];
            const bool bPossiblyDuplicatedModification = notification.Action == EFileTrackingAction::Modified && notification.FileSize == 0;

            if (bPossiblyDuplicatedModification)
            {
                if (buffer.size() == 1)
                {
                    bBufferProcessingPostponed = true;
                    break;
                }

                if (idx + 1 < buffer.size())
                {
                    const auto& nextNotification = buffer[idx + 1];
                    const bool bDuplicatedModification = nextNotification.Action == EFileTrackingAction::Modified &&
                                                         nextNotification.Path == notification.Path;
                    if (bDuplicatedModification)
                    {
                        continue;
                    }
                }
            }

            ProcessNotification(notification);
        }

        if (!bBufferProcessingPostponed)
        {
            buffer.clear();
        }
    }

    bool AssetMonitor::HasExpiredAssets() const
    {
        return !expiredAssetInfos.empty();
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
                }

                ++directoryItr;
            }
        }
    }

    void AssetMonitor::StartTracking()
    {
        auto& event = tracker->GetEvent();
        event.Subscribe("AssetMonitor"_fs,
                        [this](const FileNotification& newNotification)
                        {
                            ReadWriteLock rwLock{ bufferMutex };
                            buffer.emplace_back(newNotification);
                        });

        const EFileTrackerStatus status = tracker->StartTracking(fs::path{ details::AssetRootPath }, EFileTrackingMode::Event);

        IG_LOG(AssetMonitor, Info, "Start Asset Tracking Status: {}", magic_enum::enum_name(status));
        IG_CHECK(status == EFileTrackerStatus::Success);
    }

    void AssetMonitor::ProcessNotification(const FileNotification& notification)
    {
        /* AssetMonitor는 에셋의 정보의 변화만 감시한다. */
        if (!IsMetadataPath(notification.Path))
        {
            return;
        }

        const xg::Guid guidFromPath{ ConvertMetadataPathToGuid(notification.Path) };
        if (!guidFromPath.isValid())
        {
            IG_LOG(AssetMonitor, Error, "Found invalid guid from {}.", notification.Path.string());
            return;
        }

        // 기본적으로 모두 유효한 GUID를 가지는 경우에만 가능.
        // Added/Modified/RenamedNew => 테이블에 업데이트.
        // 만약 새로운 AssetInfo가 invalid 하거나 파일 이름에 있는 guid 와 메타데이터 guid 가 일치하지 않으면 오류.
        // 오류 처리:
        // Added/RenamedNew => 업데이트 하지 않음
        // Modified => 유효한 경우 테이블에서 삭제.
        // ------------------------------------------
        // Removed/RenamedOld => 유효한 경우 테이블에서 삭제
        bool bRemoveRequired = notification.Action == EFileTrackingAction::Removed || notification.Action == EFileTrackingAction::RenamedOldName;
        if (!bRemoveRequired)
        {
            const json serializedMetadata{ LoadJsonFromFile(notification.Path) };
            AssetInfo assetInfo{};
            serializedMetadata >> assetInfo;

            bool bErrorOccurs = false;
            if (!assetInfo.IsValid())
            {
                bErrorOccurs = true;
                IG_LOG(AssetMonitor, Error, "Asset info {} is invalid.", notification.Path.string());
            }

            if (guidFromPath != assetInfo.Guid)
            {
                bErrorOccurs = true;
                IG_LOG(AssetMonitor, Error, "Guid from path {} != Asset Info Guid {}.", guidFromPath.str(), assetInfo.Guid.str());
            }

            if (!bErrorOccurs)
            {
                VirtualPathGuidTable& virtualPathGuidTable = GetVirtualPathGuidTable(assetInfo.Type);
                if (virtualPathGuidTable.contains(assetInfo.VirtualPath))
                {
                    const xg::Guid expiredGuid = virtualPathGuidTable[assetInfo.VirtualPath];
                    expiredAssetInfos.emplace_back(guidAssetInfoTable[expiredGuid]);
                    guidAssetInfoTable.erase(expiredGuid);

                    IG_LOG(AssetMonitor, Info, "Virtual path duplication found: {}. The old asset {} info has been expired.",
                           assetInfo.VirtualPath.ToStringView(), expiredGuid.str());
                }

                virtualPathGuidTable.insert_or_assign(assetInfo.VirtualPath, assetInfo.Guid);
                guidAssetInfoTable.insert_or_assign(assetInfo.Guid, assetInfo);

                IG_LOG(AssetMonitor, Info, "{} => [Guid: {}, Virtual Path: {}, Type: {}] cached.",
                       magic_enum::enum_name(notification.Action), assetInfo.Guid.str(), assetInfo.VirtualPath.ToStringView(), magic_enum::enum_name(assetInfo.Type));
            }
            else
            {
                IG_LOG(AssetMonitor, Error, "Error Occurs({})", magic_enum::enum_name(notification.Action));
                bRemoveRequired = true;
            }
        }

        if (bRemoveRequired && guidAssetInfoTable.contains(guidFromPath))
        {
            const AssetInfo cachedAssetInfo = guidAssetInfoTable[guidFromPath];
            VirtualPathGuidTable& virtualPathGuidTable = GetVirtualPathGuidTable(cachedAssetInfo.Type);
            IG_LOG(AssetMonitor, Error, "{} => [Guid: {}, Virtual Path: {}, Type: {}] removed.",
                   magic_enum::enum_name(notification.Action), cachedAssetInfo.Guid.str(), cachedAssetInfo.VirtualPath.ToStringView(), magic_enum::enum_name(cachedAssetInfo.Type));

            virtualPathGuidTable.erase(cachedAssetInfo.VirtualPath);
            guidAssetInfoTable.erase(cachedAssetInfo.Guid);
        }
    }

    std::vector<AssetInfo> AssetMonitor::FlushExpiredAssetInfos()
    {
        std::vector<AssetInfo> cloneOfAssetInfos{ expiredAssetInfos };
        expiredAssetInfos.clear();
        return cloneOfAssetInfos;
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
} // namespace ig