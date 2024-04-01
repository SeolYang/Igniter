#pragma once
#include <Igniter.h>
#include <Asset/Common.h>

namespace ig::details
{
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
        [[nodiscard]] T::LoadDesc GetLoadDesc(const xg::Guid guid) const
        {
            ReadOnlyLock lock{ mutex };
            return GetLoadDescUnsafe<T>(guid);
        }

        template <Asset T>
        [[nodiscard]] T::LoadDesc GetLoadDesc(const String virtualPath) const
        {
            ReadOnlyLock lock{ mutex };
            IG_CHECK(ContainsUnsafe(AssetTypeOf_v<T>, virtualPath));
            return GetLoadDescUnsafe<T>(GetGuidUnsafe(AssetTypeOf_v<T>, virtualPath));
        }

        template <Asset T>
        [[nodiscard]] T::Desc GetDesc(const xg::Guid guid) const
        {
            ReadOnlyLock lock{ mutex };
            return GetDescUnsafe<T>(guid);
        }

        template <Asset T>
        [[nodiscard]] T::Desc GetDesc(const String virtualPath) const
        {
            ReadOnlyLock lock{ mutex };
            IG_CHECK(ContainsUnsafe(AssetTypeOf_v<T>, virtualPath));
            return GetDescUnsafe<T>(GetGuidUnsafe(AssetTypeOf_v<T>, virtualPath));
        }

        template <Asset T>
        void Create(const AssetInfo& newInfo, const typename T::LoadDesc& metadata)
        {
            ReadWriteLock rwLock{ mutex };
            IG_CHECK(newInfo.IsValid());
            IG_CHECK(!ContainsUnsafe(newInfo.Guid));

            VirtualPathGuidTable& virtualPathGuidTable = GetVirtualPathGuidTable(newInfo.Type);
            IG_CHECK(!virtualPathGuidTable.contains(newInfo.VirtualPath));

            json newSerialized{};
            newSerialized << newInfo;
            newSerialized << metadata;
            guidSerializedDescTable[newInfo.Guid] = newSerialized;
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

        [[nodiscard]] bool ContainsUnsafe(const xg::Guid guid) const;
        [[nodiscard]] bool ContainsUnsafe(const EAssetType assetType, const String virtualPath) const;

        [[nodiscard]] xg::Guid GetGuidUnsafe(const EAssetType assetType, const String virtualPath) const;
        [[nodiscard]] AssetInfo GetAssetInfoUnsafe(const xg::Guid guid) const;

        template <Asset T>
        [[nodiscard]] T::LoadDesc GetLoadDescUnsafe(const xg::Guid guid) const
        {
            IG_CHECK(ContainsUnsafe(guid));

            const json& serializedDesc = guidSerializedDescTable[guid];
            typename T::LoadDesc loadDesc{};
            serializedDesc >> loadDesc;

            return loadDesc;
        }

        template <Asset T>
        [[nodiscard]] T::Desc GetDescUnsafe(const xg::Guid guid) const
        {
            IG_CHECK(ContainsUnsafe(guid));

            const json& serializedDesc{ guidSerializedDescTable.at(guid) };
            typename T::Desc desc{};
            serializedDesc >> desc.Info >> desc.LoadDescriptor;

            return desc;
        }

        void ReflectExpiredToFilesUnsafe();
        void ReflectRemainedToFilesUnsafe();

    private:
        mutable SharedMutex mutex;
        std::vector<std::pair<EAssetType, VirtualPathGuidTable>> virtualPathGuidTables;
        robin_hood::unordered_map<xg::Guid, json> guidSerializedDescTable;
        robin_hood::unordered_map<xg::Guid, AssetInfo> expiredAssetInfos;
    };
} // namespace ig::details