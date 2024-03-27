#pragma once
#include <Igniter.h>
#include <Asset/Common.h>

namespace ig
{
    class AsyncFileTracker;
    struct FileNotification;
    class AssetInfoMonitor
    {
        using VirtualPathGuidTable = robin_hood::unordered_map<String, xg::Guid>;

    public:
        AssetInfoMonitor();
        ~AssetInfoMonitor();

        void ProcessBufferedNotifications();
        [[nodiscard]] bool HasExpiredAssets() const;
        [[nodiscard]] std::vector<AssetInfo> FlushExpiredAssetInfos();

        [[nodiscard]] bool Contains(const xg::Guid guid) const;
        [[nodiscard]] bool Contains(const EAssetType assetType, const String virtualPath) const;

        [[nodiscard]] xg::Guid GetGuid(const EAssetType assetType, const String virtualPath) const;

        [[nodiscard]] std::optional<AssetInfo> GetAssetInfo(const xg::Guid guid) const;
        [[nodiscard]] std::optional<AssetInfo> GetAssetInfo(const EAssetType assetType, const String virtualPath) const;

    private:
        VirtualPathGuidTable& GetVirtualPathGuidTable(const EAssetType assetType);
        const VirtualPathGuidTable& GetVirtualPathGuidTable(const EAssetType assetType) const;

        void InitVirtualPathGuidTables();
        void ParseAssetDirectory();
        void StartTracking();

        void ProcessNotification(const FileNotification& notification);

    private:
        std::unique_ptr<AsyncFileTracker> tracker;

        SharedMutex bufferMutex;
        std::vector<FileNotification> buffer;

        std::vector<std::pair<EAssetType, VirtualPathGuidTable>> virtualPathGuidTables;
        robin_hood::unordered_map<xg::Guid, AssetInfo> guidAssetInfoTable;

        constexpr static size_t ReservedExpiredAssetInfoBufferSize = 64;
        std::vector<AssetInfo> expiredAssetInfos;
    };
} // namespace ig