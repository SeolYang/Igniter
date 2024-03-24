#include <Igniter.h>
#include <Core/Log.h>
#include <Core/Serializable.h>
#include <Core/Event.h>
#include <Filesystem/Utils.h>
#include <Filesystem/AsyncFileTracker.h>
#include <Asset/AssetManager.h>
#include <Asset/Utils.h>

namespace ig
{
    IG_DEFINE_LOG_CATEGORY(AssetManagerInfo, ELogVerbosity::Info)
    IG_DEFINE_LOG_CATEGORY(AssetManagerWarn, ELogVerbosity::Warning)
    IG_DEFINE_LOG_CATEGORY(AssetManagerErr, ELogVerbosity::Error)
    IG_DEFINE_LOG_CATEGORY(AssetManagerDbg, ELogVerbosity::Debug)
    IG_DEFINE_LOG_CATEGORY(AssetManagerFatal, ELogVerbosity::Fatal)

    AssetManager::AssetManager() : assetDirTracker(std::make_unique<AsyncFileTracker>())
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

        ParseInitialAssetDirectory();

        auto& event = assetDirTracker->GetEvent();
        event.Subscribe("AssetManager"_fs,
                        [this](const FileNotification& newNotification)
                        {
                            ReadWriteLock rwLock{ bufferMutex };
                            notificationBuffer.emplace_back(newNotification);
                        });

        const EFileTrackerResult trackingResult = assetDirTracker->StartTracking(fs::path{ details::AssetRootPath },
                                                                                 EFileTrackingMode::Event,
                                                                                 ETrackingFilterFlags::Default, true,
                                                                                 66ms);
        IG_LOG(AssetManagerInfo, "Asset Dir Tracker Status: {}", magic_enum::enum_name(trackingResult));
        IG_CHECK(trackingResult == EFileTrackerResult::Success);
    }

    AssetManager::~AssetManager()
    {
    }

    void AssetManager::ProcessFileNotifications()
    {
        ReadWriteLock rwLock{ bufferMutex };
        for (const auto& notification : notificationBuffer)
        {
            ProcessFileNotification(notification);
        }
        notificationBuffer.clear();
    }

    AssetManager::VirtualPathGuidTable& AssetManager::GetVirtualPathGuidTable(const EAssetType assetType)
    {
        /* #sy_log Unknown 의 경우에도 테이블을 가져올 수 있긴 하다. 이상적으로는 없는게 좋지만, 간결함을 위해 유지 할 것. */
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

        /* #sy_log 생성자에서 항상 모든 asset type 에 대해 테이블을 생성하기에,
        assetType에 완전 무의미한 값을 캐스팅 해서 넣는게 아닌 이상 항상 유효하다.*/
        IG_CHECK(table != nullptr);
        return *table;
    }

    void AssetManager::ParseInitialAssetDirectory()
    {
        for (const auto assetType : magic_enum::enum_values<EAssetType>())
        {
            if (assetType == EAssetType::Unknown)
            {
                continue;
            }

            VirtualPathGuidTable& virtualPathGuidTable = GetVirtualPathGuidTable(assetType);
            fs::directory_iterator directoryItr{ GetAssetDirectoryPath(assetType) };

            IG_LOG(AssetManagerDbg, "Root Dir: {}", GetAssetDirectoryPath(assetType).string());
            while (directoryItr != fs::end(directoryItr))
            {
                const fs::directory_entry& entry = *directoryItr;
                if (entry.is_regular_file() && !entry.path().has_extension())
                {
                    /* 최초에 온건한 형식의 메타데이터만 캐싱 됨을 보장 하여야 함! */
                    const xg::Guid guidFromPath{ entry.path().filename().string() };
                    if (!guidFromPath.isValid())
                    {
                        IG_LOG(AssetManagerWarn, "Asset {} ignored. {} is not valid guid.", entry.path().string(), entry.path().filename().string());
                        ++directoryItr;
                        continue;
                    }

                    fs::path metadataPath{ entry.path() };
                    metadataPath.replace_extension(details::MetadataExt);
                    if (!fs::exists(metadataPath))
                    {
                        IG_LOG(AssetManagerWarn, "Asset {} ignored. The metadata does not exists.", entry.path().string());
                        ++directoryItr;
                        continue;
                    }

                    const json serializedMetadata{ LoadJsonFromFile(metadataPath) };
                    AssetInfo assetInfo{};
                    serializedMetadata >> assetInfo;

                    if (!assetInfo.IsValid())
                    {
                        IG_LOG(AssetManagerWarn, "Asset {} ignored. The asset info is invalid.", entry.path().string());
                        ++directoryItr;
                        continue;
                    }

                    if (guidFromPath != assetInfo.Guid)
                    {
                        IG_LOG(AssetManagerWarn, "Asset {} ignored. The guid from filename does not match asset info guid.", entry.path().string());
                        ++directoryItr;
                        continue;
                    }

                    IG_CHECK(!virtualPathGuidTable.contains(assetInfo.VirtualPath));
                    virtualPathGuidTable.insert_or_assign(assetInfo.VirtualPath, assetInfo.Guid);
                    IG_LOG(AssetManagerDbg, "VirtualPath: {}, Guid: {}", assetInfo.VirtualPath.AsStringView(), assetInfo.Guid.str());
                    IG_CHECK(!guidAssetInfoTable.contains(assetInfo.Guid));
                    guidAssetInfoTable.insert_or_assign(assetInfo.Guid, assetInfo);
                }

                ++directoryItr;
            }
        }
    }

    void AssetManager::ProcessFileNotification(const FileNotification& notification)
    {
        if (!IsMetadataPath(notification.Path))
        {
            /* 메타데이터를 유지하기 위함으로, 메타데이터 이외는 신경 쓰지 않는다. */
            /* 에셋 파일이 없어서 실패하는 경우는, Load 함수들에서 처리해야함. */
            return;
        }

        const xg::Guid guidFromPath{ ConvertMetadataPathToGuid(notification.Path) };
        if (!guidFromPath.isValid())
        {
            IG_LOG(AssetManagerErr, "Found invalid guid from {}.", notification.Path.string());
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
                IG_LOG(AssetManagerErr, "Asset info {} is invalid.", notification.Path.string());
            }

            if (guidFromPath != assetInfo.Guid)
            {
                bErrorOccurs = true;
                IG_LOG(AssetManagerErr, "Guid from path {} != Asset Info Guid {}.", guidFromPath.str(), assetInfo.Guid.str());
            }

            if (!bErrorOccurs)
            {
                // 삭제의 경우 캐시되어 있지 않으면 무시하는 방식으로 중복 처리가 가능하지만
                // 제대로된 데이터를 캐싱하는 경우에는 중복되어 들어온 이벤트 간의 '차이'를 알 수가 없음.
                IG_LOG(AssetManagerInfo, "{} => [Guid: {}, Virtual Path: {}, Type: {}] cached.",
                       magic_enum::enum_name(notification.Action), assetInfo.Guid.str(), assetInfo.VirtualPath.AsStringView(), magic_enum::enum_name(assetInfo.Type));

                VirtualPathGuidTable& virtualPathGuidTable = GetVirtualPathGuidTable(assetInfo.Type);
                virtualPathGuidTable.insert_or_assign(assetInfo.VirtualPath, assetInfo.Guid);
                guidAssetInfoTable.insert_or_assign(assetInfo.Guid, assetInfo);
            }
            else
            {
                IG_LOG(AssetManagerErr, "Error Occurs({})", magic_enum::enum_name(notification.Action));
                bRemoveRequired = true;
            }
        }

        // 원래 캐시되어 있었던 경우, 항상 파일에 저장된 데이터는 '유효'
        // 하지만, 중복 처리는 막을 것.
        if (bRemoveRequired && guidAssetInfoTable.contains(guidFromPath))
        {
            const AssetInfo cachedAssetInfo = guidAssetInfoTable[guidFromPath];
            VirtualPathGuidTable& virtualPathGuidTable = GetVirtualPathGuidTable(cachedAssetInfo.Type);
            IG_LOG(AssetManagerInfo, "{} => [Guid: {}, Virtual Path: {}, Type: {}] removed.",
                   magic_enum::enum_name(notification.Action), cachedAssetInfo.Guid.str(), cachedAssetInfo.VirtualPath.AsStringView(), magic_enum::enum_name(cachedAssetInfo.Type));

            virtualPathGuidTable.erase(cachedAssetInfo.VirtualPath);
            guidAssetInfoTable.erase(cachedAssetInfo.Guid);
        }
    }
} // namespace ig