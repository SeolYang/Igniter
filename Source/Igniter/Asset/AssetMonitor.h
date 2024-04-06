#pragma once
#include <Igniter.h>
#include <Asset/Common.h>

namespace ig::details
{
    class AssetMonitor
    {
        using VirtualPathGuidTable = UnorderedMap<String, Guid>;

    public:
        AssetMonitor();
        ~AssetMonitor();

        [[nodiscard]] bool Contains(const Guid guid) const;
        [[nodiscard]] bool Contains(const EAssetType assetType, const String virtualPath) const;

        [[nodiscard]] Guid GetGuid(const EAssetType assetType, const String virtualPath) const;
        [[nodiscard]] AssetInfo GetAssetInfo(const Guid guid) const;
        [[nodiscard]] AssetInfo GetAssetInfo(const EAssetType assetType, const String virtualPath) const;

        template <Asset T>
        [[nodiscard]] T::LoadDesc GetLoadDesc(const Guid guid) const
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
        [[nodiscard]] void SetLoadDesc(const Guid guid, const typename T::LoadDesc& loadDesc)
        {
             ReadWriteLock rwLock{ mutex };
             SetLoadDescUnsafe<T>(guid, loadDesc);
        }

        template <Asset T>
        [[nodiscard]] void SetLoadDesc(const String virtualPath, const typename T::LoadDesc& loadDesc)
        {
            ReadWriteLock rwLock{ mutex };
            IG_CHECK(ContainsUnsafe(AssetTypeOf_v<T>, virtualPath));
            SetLoadDescUnsafe<T>(GetGuidUnsafe(AssetTypeOf_v<T>, virtualPath), loadDesc);
        }

        template <Asset T>
        [[nodiscard]] T::Desc GetDesc(const Guid guid) const
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
        void Create(const AssetInfo& newInfo, const typename T::LoadDesc& loadDesc)
        {
            ReadWriteLock rwLock{ mutex };
            IG_CHECK(newInfo.IsValid());
            IG_CHECK(!ContainsUnsafe(newInfo.GetGuid()));

            const Guid guid{ newInfo.GetGuid() };
            const String virtualPath{ newInfo.GetVirtualPath() };
            IG_CHECK(IsValidVirtualPath(virtualPath));

            VirtualPathGuidTable& virtualPathGuidTable = GetVirtualPathGuidTable(newInfo.GetType());
            IG_CHECK(!virtualPathGuidTable.contains(virtualPath));

            json newSerialized{};
            newSerialized << newInfo << loadDesc;
            guidSerializedDescTable[guid] = newSerialized;
            virtualPathGuidTable[virtualPath] = guid;
        }

        void UpdateInfo(const AssetInfo& newInfo);
        void Remove(const Guid guid);
        void SaveAllChanges();

        [[nodiscard]] std::vector<AssetInfo> TakeSnapshots() const;

    private:
        VirtualPathGuidTable& GetVirtualPathGuidTable(const EAssetType assetType);
        const VirtualPathGuidTable& GetVirtualPathGuidTable(const EAssetType assetType) const;

        void InitVirtualPathGuidTables();
        void ParseAssetDirectory();

        [[nodiscard]] bool ContainsUnsafe(const Guid guid) const;
        [[nodiscard]] bool ContainsUnsafe(const EAssetType assetType, const String virtualPath) const;

        [[nodiscard]] Guid GetGuidUnsafe(const EAssetType assetType, const String virtualPath) const;
        [[nodiscard]] AssetInfo GetAssetInfoUnsafe(const Guid guid) const;

        template <Asset T>
        [[nodiscard]] T::LoadDesc GetLoadDescUnsafe(const Guid guid) const
        {
            IG_CHECK(ContainsUnsafe(guid));

            const json& serializedDesc = guidSerializedDescTable[guid];
            typename T::LoadDesc loadDesc{};
            serializedDesc >> loadDesc;

            return loadDesc;
        }

        template <Asset T>
        void SetLoadDescUnsafe(const Guid guid, typename T::LoadDesc& loadDesc)
        {
            IG_CHECK(ContainsUnsafe(guid));

            json& serializedDesc{ guidSerializedDescTable[guid] };
            serializedDesc << loadDesc;
        }

        template <Asset T>
        [[nodiscard]] T::Desc GetDescUnsafe(const Guid guid) const
        {
            IG_CHECK(ContainsUnsafe(guid));

            const json& serializedDesc{ guidSerializedDescTable.at(guid) };
            typename T::Desc desc{};
            serializedDesc >> desc.Info >> desc.LoadDescriptor;

            return desc;
        }

        void ReflectExpiredToFilesUnsafe();
        void ReflectRemainedToFilesUnsafe();
        void CleanupOrphanFiles();

    private:
        mutable SharedMutex mutex;
        std::vector<std::pair<EAssetType, VirtualPathGuidTable>> virtualPathGuidTables;
        UnorderedMap<Guid, json> guidSerializedDescTable;
        UnorderedMap<Guid, AssetInfo> expiredAssetInfos;
    };
} // namespace ig::details