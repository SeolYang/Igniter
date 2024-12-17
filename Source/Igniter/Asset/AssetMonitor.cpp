#include "Igniter/Igniter.h"
#include "Igniter/Core/Log.h"
#include "Igniter/Filesystem/Utils.h"
#include "Igniter/Asset/Texture.h"
#include "Igniter/Asset/StaticMesh.h"
#include "Igniter/Asset/Material.h"
#include "Igniter/Asset/Map.h"
#include "Igniter/Asset/AssetMonitor.h"

IG_DEFINE_LOG_CATEGORY(AssetMonitor);

namespace ig::details
{
    AssetMonitor::AssetMonitor()
    {
        InitAssetDescTables();
        InitVirtualPathGuidTables();
        CleanupOrphanFiles();
        ParseAssetDirectory();
    }

    AssetMonitor::~AssetMonitor() {}

    void AssetMonitor::InitAssetDescTables()
    {
        guidDescTables.emplace_back(std::make_pair(EAssetCategory::Texture, MakePtr<AssetDescMap<Texture>>()));
        guidDescTables.emplace_back(std::make_pair(EAssetCategory::StaticMesh, MakePtr<AssetDescMap<StaticMesh>>()));
        guidDescTables.emplace_back(std::make_pair(EAssetCategory::Material, MakePtr<AssetDescMap<Material>>()));
        guidDescTables.emplace_back(std::make_pair(EAssetCategory::Map, MakePtr<AssetDescMap<Map>>()));
    }

    void AssetMonitor::InitVirtualPathGuidTables()
    {
        for (const auto assetType : magic_enum::enum_values<EAssetCategory>())
        {
            virtualPathGuidTables.emplace_back(assetType, VirtualPathGuidTable{});

            if (assetType != EAssetCategory::Unknown)
            {
                const Path typeDirPath{ GetAssetDirectoryPath(assetType) };
                if (!fs::exists(typeDirPath))
                {
                    fs::create_directories(typeDirPath);
                }
            }
        }
    }

    void AssetMonitor::ParseAssetDirectory()
    {
        for (const auto assetType : magic_enum::enum_values<EAssetCategory>())
        {
            if (assetType == EAssetCategory::Unknown)
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
                        IG_LOG(
                            AssetMonitor, Error, "Asset {} ignored. {} is not valid guid.", entry.path().string(), entry.path().filename().string());
                        ++directoryItr;
                        continue;
                    }

                    Path metadataPath{ entry.path() };
                    metadataPath.replace_extension(details::MetadataExt);
                    if (!fs::exists(metadataPath))
                    {
                        IG_LOG(AssetMonitor, Error, "Asset {} ignored. The metadata does not exists.", entry.path().string());
                        ++directoryItr;
                        continue;
                    }

                    Json serializedMetadata{ LoadJsonFromFile(metadataPath) };
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
                        IG_LOG(AssetMonitor, Error,
                               "{}: Asset {} ignored. The guid from filename does not match asset info guid."
                               " Which was {}.",
                               assetInfo.GetCategory(), entry.path().string(), guid);
                        ++directoryItr;
                        continue;
                    }

                    if (virtualPathGuidTable.contains(virtualPath))
                    {
                        IG_LOG(AssetMonitor, Error, "{}: Asset {} ({}) ignored. Which has duplicated virtual path.", assetInfo.GetCategory(),
                               virtualPath, guid);
                        ++directoryItr;
                        continue;
                    }

                    if (assetInfo.GetScope() == EAssetScope::Engine)
                    {
                        IG_LOG(AssetMonitor, Warning, "{}: Found invalid asset scope Asset {} ({}). Assumes as Managed.", assetInfo.GetCategory(),
                               virtualPath, guid);
                        assetInfo.SetScope(EAssetScope::Managed);
                        serializedMetadata << assetInfo;
                    }

                    virtualPathGuidTable[virtualPath] = guid;
                    IG_LOG(AssetMonitor, Debug, "VirtualPath: {}, Guid: {}", virtualPath, guid);
                    IG_CHECK(!Contains(guid));
                    TypelessAssetDescMap& descTable{ GetDescMap(assetInfo.GetCategory()) };
                    descTable.Insert(serializedMetadata);
                }

                ++directoryItr;
            }
        }
    }

    AssetMonitor::VirtualPathGuidTable& AssetMonitor::GetVirtualPathGuidTable(const EAssetCategory assetType)
    {
        IG_CHECK(assetType != EAssetCategory::Unknown);
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

    const AssetMonitor::VirtualPathGuidTable& AssetMonitor::GetVirtualPathGuidTable(const EAssetCategory assetType) const
    {
        IG_CHECK(assetType != EAssetCategory::Unknown);
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

    bool AssetMonitor::ContainsUnsafe(const Guid& guid) const
    {
        for (const auto& typeDescTablePair : guidDescTables)
        {
            if (typeDescTablePair.second->Contains(guid))
            {
                return true;
            }
        }

        return false;
    }

    bool AssetMonitor::ContainsUnsafe(const EAssetCategory assetType, const String virtualPath) const
    {
        IG_CHECK(assetType != EAssetCategory::Unknown);

        const VirtualPathGuidTable& virtualPathGuidTable = GetVirtualPathGuidTable(assetType);
        const auto itr = virtualPathGuidTable.find(virtualPath);
        return itr != virtualPathGuidTable.cend() && ContainsUnsafe(itr->second);
    }

    Guid AssetMonitor::GetGuidUnsafe(const EAssetCategory assetType, const String virtualPath) const
    {
        IG_CHECK(assetType != EAssetCategory::Unknown);
        IG_CHECK(IsValidVirtualPath(virtualPath));

        const VirtualPathGuidTable& virtualPathGuidTable = GetVirtualPathGuidTable(assetType);
        const auto itr = virtualPathGuidTable.find(virtualPath);
        IG_CHECK(itr != virtualPathGuidTable.cend());

        const Guid guid{ itr->second };
        IG_CHECK(guid.isValid());

        return guid;
    }

    bool AssetMonitor::Contains(const Guid& guid) const
    {
        ReadOnlyLock lock{ mutex };
        return ContainsUnsafe(guid);
    }

    bool AssetMonitor::Contains(const EAssetCategory assetType, const String virtualPath) const
    {
        ReadOnlyLock lock{ mutex };
        return ContainsUnsafe(assetType, virtualPath);
    }

    Guid AssetMonitor::GetGuid(const EAssetCategory assetType, const String virtualPath) const
    {
        ReadOnlyLock lock{ mutex };
        return GetGuidUnsafe(assetType, virtualPath);
    }

    AssetInfo AssetMonitor::GetAssetInfoUnsafe(const Guid& guid) const
    {
        IG_CHECK(ContainsUnsafe(guid));

        for (const auto& typeDescTablePair : guidDescTables)
        {
            const TypelessAssetDescMap& descMap{ *typeDescTablePair.second };
            if (descMap.Contains(guid))
            {
                return descMap.GetAssetInfo(guid);
            }
        }

        IG_CHECK_NO_ENTRY();
        return {};
    }

    AssetInfo AssetMonitor::GetAssetInfo(const Guid& guid) const
    {
        ReadOnlyLock lock{ mutex };
        return GetAssetInfoUnsafe(guid);
    }

    AssetInfo AssetMonitor::GetAssetInfo(const EAssetCategory assetType, const String virtualPath) const
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
        IG_CHECK(newInfo.GetCategory() == oldInfo.GetCategory());

        VirtualPathGuidTable& virtualPathGuidTable = GetVirtualPathGuidTable(newInfo.GetCategory());
        if (virtualPath != oldVirtualPath)
        {
            IG_CHECK(!virtualPathGuidTable.contains(virtualPath));
            virtualPathGuidTable.erase(oldVirtualPath);
            virtualPathGuidTable[virtualPath] = guid;
        }

        TypelessAssetDescMap& descMap{ GetDescMap(newInfo.GetCategory()) };
        descMap.Update(newInfo);
    }

    void AssetMonitor::Remove(const Guid& guid, const bool bShouldExpired)
    {
        ReadWriteLock rwLock{ mutex };
        IG_CHECK(ContainsUnsafe(guid));

        const AssetInfo info = GetAssetInfoUnsafe(guid);
        IG_CHECK(info.IsValid());
        IG_CHECK(info.GetGuid() == guid);

        const String virtualPath{ info.GetVirtualPath() };

        VirtualPathGuidTable& virtualPathGuidTable = GetVirtualPathGuidTable(info.GetCategory());
        IG_CHECK(virtualPathGuidTable.contains(virtualPath));
        IG_CHECK(virtualPathGuidTable[virtualPath] == guid);

        if (bShouldExpired)
        {
            expiredAssetInfos[guid] = info;
        }

        virtualPathGuidTable.erase(virtualPath);

        for (auto& assetTypeDescTablePair : guidDescTables)
        {
            TypelessAssetDescMap& descMap{ *assetTypeDescTablePair.second };
            if (descMap.Contains(guid))
            {
                descMap.Erase(guid);
            }
        }
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
            const Path metadataPath{ MakeAssetMetadataPath(expiredAssetInfo.GetCategory(), expiredGuid) };
            const Path assetPath{ MakeAssetPath(expiredAssetInfo.GetCategory(), expiredGuid) };
            if (fs::exists(metadataPath))
            {
                IG_ENSURE(fs::remove(metadataPath));
            }

            if (fs::exists(assetPath))
            {
                IG_ENSURE(fs::remove(assetPath));
            }

            IG_LOG(AssetMonitor, Debug, "{} Asset Expired: {} ({})", expiredAssetInfo.GetCategory(), expiredVirtualPath, expiredGuid);
        }

        expiredAssetInfos.clear();
    }

    void AssetMonitor::ReflectRemainedToFilesUnsafe()
    {
        IG_CHECK(expiredAssetInfos.empty());
        for (const auto& assetTypeDescTablePair : guidDescTables)
        {
            TypelessAssetDescMap& descMap{ *assetTypeDescTablePair.second };
            std::vector<Json> serializedDescs{ descMap.GetSerializedDescs() };
            for (const Json& serializedDesc : serializedDescs)
            {
                AssetInfo assetInfo{};
                serializedDesc >> assetInfo;
                IG_CHECK(assetInfo.IsValid());

                if (assetInfo.GetScope() != EAssetScope::Engine)
                {
                    const Guid guid{ assetInfo.GetGuid() };
                    const String virtualPath{ assetInfo.GetVirtualPath() };

                    const Path metadataPath{ MakeAssetMetadataPath(assetInfo.GetCategory(), guid) };
                    IG_ENSURE(SaveJsonToFile(metadataPath, serializedDesc));
                    IG_LOG(AssetMonitor, Debug, "{} Asset metadata Saved: {} ({})", assetInfo.GetCategory(), virtualPath, guid);
                }
            }
        }
    }

    void AssetMonitor::CleanupOrphanFiles()
    {
        std::vector<Path> orphanFiles{};
        for (const auto assetType : magic_enum::enum_values<EAssetCategory>())
        {
            if (assetType == EAssetCategory::Unknown)
            {
                continue;
            }

            fs::directory_iterator directoryItr{ GetAssetDirectoryPath(assetType) };
            while (directoryItr != fs::end(directoryItr))
            {
                Path path{ directoryItr->path() };
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

        for (const Path& orphanFilePath : orphanFiles)
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
        size_t numDescs{ 0 };

        for (const auto& typeDescTablePair : guidDescTables)
        {
            const TypelessAssetDescMap& descMap{ *typeDescTablePair.second };
            numDescs += descMap.GetSize();
        }

        std::vector<AssetInfo> assetInfoSnapshots{};
        assetInfoSnapshots.reserve(numDescs);
        for (const auto& typeDescTablePair : guidDescTables)
        {
            const TypelessAssetDescMap& descMap{ *typeDescTablePair.second };
            std::vector<AssetInfo> assetInfos{ descMap.GetAssetInfos() };
            assetInfoSnapshots.insert(assetInfoSnapshots.end(), assetInfos.begin(), assetInfos.end());
        }

        return assetInfoSnapshots;
    }

    TypelessAssetDescMap& AssetMonitor::GetDescMap(const EAssetCategory assetType)
    {
        TypelessAssetDescMap* ptr{ nullptr };
        for (auto& typeMapPair : guidDescTables)
        {
            if (typeMapPair.first == assetType)
            {
                ptr = typeMapPair.second.get();
                break;
            }
        }
        IG_CHECK(ptr != nullptr);
        return *ptr;
    }

    const TypelessAssetDescMap& AssetMonitor::GetDescMap(const EAssetCategory assetType) const
    {
        TypelessAssetDescMap* ptr{ nullptr };
        for (auto& typeMapPair : guidDescTables)
        {
            if (typeMapPair.first == assetType)
            {
                ptr = typeMapPair.second.get();
                break;
            }
        }
        IG_CHECK(ptr != nullptr);
        return *ptr;
    }
}    // namespace ig::details
