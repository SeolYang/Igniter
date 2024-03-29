#pragma once
#include <Igniter.h>
#include <Core/Serializable.h>
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
        [[nodiscard]] AssetInfo GetAssetInfo(const xg::Guid guid) const;
        [[nodiscard]] AssetInfo GetAssetInfo(const EAssetType assetType, const String virtualPath) const;

        template <Asset T>
        [[nodiscard]] T::MetadataType GetMetadata(const xg::Guid guid) const
        {
            IG_CHECK(Contains(guid));

            const json& serializedMeta = guidSerializedMetaTable[guid];
            typename T::MetadataType result{};
            serializedMeta >> result;

            return result;
        }

        template <Asset T>
        [[nodiscard]] T::MetadataType GetMetadata(const String virtualPath) const
        {
            return GetMetadata<T>(GetGuid(AssetTypeOf_v<T>, virtualPath));
        }

        template <Asset T>
        void SetMetadata(const xg::Guid guid, const typename T::MetadataType& metadata)
        {
            IG_CHECK(guid.isValid());
            IG_CHECK(Contains(guid));

            json& serializedMeta = guidSerializedMetaTable[guid];
            serializedMeta << metadata;
        }

        template <Asset T>
        void SetMetadata(const String virtualPath, const typename T::MetadataType& metadata)
        {
            SetMetadata<T>(GetGuid(AssetTypeOf_v<T>, virtualPath), metadata);
        }

        template <Asset T>
        void Create(const AssetInfo& newInfo, const typename T::MetadataType& metadata)
        {
            IG_CHECK(newInfo.IsValid());
            IG_CHECK(!Contains(newInfo.Guid));

            VirtualPathGuidTable& virtualPathGuidTable = GetVirtualPathGuidTable(newInfo.Type);
            IG_CHECK(!virtualPathGuidTable.contains(newInfo.VirtualPath));

            json newSerialized{};
            newSerialized << newInfo;
            newSerialized << metadata;
            guidSerializedMetaTable[newInfo.Guid] = newSerialized;
            virtualPathGuidTable[newInfo.VirtualPath] = newInfo.Guid;
        }

        void UpdateInfo(const AssetInfo& newInfo);
        void Remove(const xg::Guid guid);
        void SaveAllChanges();

    private:
        VirtualPathGuidTable& GetVirtualPathGuidTable(const EAssetType assetType);
        const VirtualPathGuidTable& GetVirtualPathGuidTable(const EAssetType assetType) const;

        void InitVirtualPathGuidTables();
        void ParseAssetDirectory();

        void ReflectExpiredToFiles();
        void ReflectRemainedToFiles();

    private:
        std::vector<std::pair<EAssetType, VirtualPathGuidTable>> virtualPathGuidTables;
        robin_hood::unordered_map<xg::Guid, json> guidSerializedMetaTable; // 오히려, 얘 자체를 들고있는 것도 나쁘진 않겠는데?
        robin_hood::unordered_map<xg::Guid, AssetInfo> expiredAssetInfos;

    };
} // namespace ig