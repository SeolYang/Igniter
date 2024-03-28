#pragma once
#include <Igniter.h>
#include <Filesystem/CoFileWatcher.h>
#include <Asset/Common.h>

namespace ig
{
    class CoFileWatcher;
    struct FileChangeInfo;
    class AssetMonitor
    {
        using VirtualPathGuidTable = robin_hood::unordered_map<String, xg::Guid>;

    public:
        AssetMonitor();
        ~AssetMonitor();

        [[nodiscard]] bool Contains(const xg::Guid guid) const;
        [[nodiscard]] bool Contains(const EAssetType assetType, const String virtualPath) const;

        [[nodiscard]] xg::Guid GetGuid(const EAssetType assetType, const String virtualPath) const;
        [[nodiscard]] std::optional<AssetInfo> GetAssetInfo(const xg::Guid guid) const;
        [[nodiscard]] std::optional<AssetInfo> GetAssetInfo(const EAssetType assetType, const String virtualPath) const;

        void Add(const AssetInfo& newInfo);
        void Update(const AssetInfo& newInfo);
        void Remove(const AssetInfo& info);
        void ReflectAllChanges();

    private:
        VirtualPathGuidTable& GetVirtualPathGuidTable(const EAssetType assetType);
        const VirtualPathGuidTable& GetVirtualPathGuidTable(const EAssetType assetType) const;

        void InitVirtualPathGuidTables();
        void ParseAssetDirectory();

        void ReflectExpiredToFiles();
        void ReflectRemainedToFiles();

    private:
        std::vector<std::pair<EAssetType, VirtualPathGuidTable>> virtualPathGuidTables;
        robin_hood::unordered_map<xg::Guid, AssetInfo> guidAssetInfoTable;
        robin_hood::unordered_map<xg::Guid, AssetInfo> expiredAssetInfos;
    };
} // namespace ig