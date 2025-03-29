#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/Serialization.h"
#include "Igniter/Asset/Common.h"

namespace ig::details
{
    class TypelessAssetDescMap
    {
    public:
        virtual ~TypelessAssetDescMap() = default;

        virtual void Insert(const Json& serializedMetadata) = 0;
        virtual void Erase(const Guid guid) = 0;
        virtual bool Contains(const Guid guid) const = 0;
        virtual Vector<Json> GetSerializedDescs() const = 0;
        virtual Vector<AssetInfo> GetAssetInfos() const = 0;
        virtual AssetInfo GetAssetInfo(const Guid guid) const = 0;
        virtual void Update(const AssetInfo& assetInfo) = 0;
        virtual Size GetSize() const = 0;
        virtual bool IsEmpty() const = 0;
    };

    template <typename T>
    class AssetDescMap : public TypelessAssetDescMap
    {
    public:
        AssetDescMap() = default;
        AssetDescMap(const AssetDescMap&) = delete;
        AssetDescMap& operator=(const AssetDescMap&) = delete;
        ~AssetDescMap() override = default;

        void Insert(const Guid guid, const typename T::Desc& newDesc) { container[guid] = newDesc; }

        void Insert(const Json& serializedMetadata) override
        {
            typename T::Desc newDesc{};
            serializedMetadata >> newDesc.Info >> newDesc.LoadDescriptor;
            container[newDesc.Info.GetGuid()] = newDesc;
        }

        void Update(const Guid guid, const typename T::LoadDesc& newLoadDesc) { container.at(guid).LoadDescriptor = newLoadDesc; }

        void Update(const AssetInfo& assetInfo) override { container.at(assetInfo.GetGuid()).Info = assetInfo; }

        void Erase(const Guid guid) override { container.erase(guid); }

        bool Contains(const Guid guid) const override { return container.contains(guid); }

        Vector<Json> GetSerializedDescs() const override
        {
            Vector<Json> result;
            result.reserve(container.size());
            for (auto guidDescPair : container)
            {
                Json serialized{};
                serialized << guidDescPair.second.Info << guidDescPair.second.LoadDescriptor;
                result.emplace_back(serialized);
            }

            return result;
        }

        Vector<AssetInfo> GetAssetInfos() const override
        {
            Vector<AssetInfo> assetInfos;
            assetInfos.reserve(GetSize());
            for (const auto& guidDescPair : container)
            {
                assetInfos.emplace_back(guidDescPair.second.Info);
            }

            return assetInfos;
        }

        AssetInfo GetAssetInfo(const Guid guid) const override { return GetDesc(guid).Info; }

        typename T::Desc GetDesc(const Guid guid) const { return container.at(guid); }

        Size GetSize() const override { return container.size(); }

        bool IsEmpty() const override { return container.empty(); }

    private:
        UnorderedMap<Guid, typename T::Desc> container{};
    };

    class AssetMonitor
    {
        using VirtualPathGuidTable = UnorderedMap<U64, Guid>;

    public:
        AssetMonitor();
        AssetMonitor(const AssetMonitor&) = delete;
        AssetMonitor(AssetMonitor&&) = delete;
        ~AssetMonitor();

        AssetMonitor& operator=(AssetMonitor&&) = delete;
        AssetMonitor& operator=(const AssetMonitor&) = delete;

        [[nodiscard]] bool Contains(const Guid& guid) const;
        [[nodiscard]] bool Contains(const EAssetCategory assetType, const std::string_view virtualPath) const;

        [[nodiscard]] Guid GetGuid(const EAssetCategory assetType, const std::string_view virtualPath) const;
        [[nodiscard]] AssetInfo GetAssetInfo(const Guid& guid) const;
        [[nodiscard]] AssetInfo GetAssetInfo(const EAssetCategory assetType, const std::string_view virtualPath) const;

        template <typename T>
        [[nodiscard]] typename T::LoadDesc GetLoadDesc(const Guid& guid) const
        {
            ReadOnlyLock lock{mutex};
            return GetLoadDescUnsafe<T>(guid);
        }

        template <typename T>
        [[nodiscard]] typename T::LoadDesc GetLoadDesc(const std::string_view virtualPath) const
        {
            ReadOnlyLock lock{mutex};
            IG_CHECK(ContainsUnsafe(AssetCategoryOf<T>, virtualPath));
            return GetLoadDescUnsafe<T>(GetGuidUnsafe(AssetCategoryOf<T>, virtualPath));
        }

        template <typename T>
        void UpdateLoadDesc(const Guid& guid, const typename T::LoadDesc& loadDesc)
        {
            ReadWriteLock rwLock{mutex};
            UpdateLoadDescUnsafe<T>(guid, loadDesc);
        }

        template <typename T>
        void UpdateLoadDesc(const std::string_view virtualPath, const typename T::LoadDesc& loadDesc)
        {
            ReadWriteLock rwLock{mutex};
            IG_CHECK(ContainsUnsafe(AssetCategoryOf<T>, virtualPath));
            UpdateLoadDescUnsafe<T>(GetGuidUnsafe(AssetCategoryOf<T>, virtualPath), loadDesc);
        }

        template <typename T>
        [[nodiscard]] T::Desc GetDesc(const Guid& guid) const
        {
            ReadOnlyLock lock{mutex};
            return GetDescUnsafe<T>(guid);
        }

        template <typename T>
        [[nodiscard]] T::Desc GetDesc(const std::string_view virtualPath) const
        {
            ReadOnlyLock lock{mutex};
            IG_CHECK(ContainsUnsafe(AssetCategoryOf<T>, virtualPath));
            return GetDescUnsafe<T>(GetGuidUnsafe(AssetCategoryOf<T>, virtualPath));
        }

        template <typename T>
        void Create(const AssetInfo& newInfo, const typename T::LoadDesc& loadDesc)
        {
            ReadWriteLock rwLock{mutex};
            IG_CHECK(newInfo.IsValid());
            IG_CHECK(!ContainsUnsafe(newInfo.GetGuid()));

            const Guid& guid{newInfo.GetGuid()};
            const std::string_view virtualPath{newInfo.GetVirtualPath()};
            const U64 virtualPathHash = Hash(virtualPath);
            IG_CHECK(IsValidVirtualPath(virtualPath));

            VirtualPathGuidTable& virtualPathGuidTable = GetVirtualPathGuidTable(newInfo.GetCategory());
            IG_CHECK(!virtualPathGuidTable.contains(virtualPathHash));

            AssetDescMap<T>& assetDescTable{GetDescMap<T>()};
            assetDescTable.Insert(guid, typename T::Desc{newInfo, loadDesc});
            virtualPathGuidTable[virtualPathHash] = guid;
        }

        void UpdateInfo(const AssetInfo& newInfo);
        void Remove(const Guid& guid, const bool bShouldExpired = true);
        void SaveAllChanges();

        [[nodiscard]] Vector<AssetInfo> TakeSnapshots(const EAssetCategory filter) const;

    private:
        void InitAssetDescTables();
        void InitVirtualPathGuidTables();
        void ParseAssetDirectory();

        VirtualPathGuidTable& GetVirtualPathGuidTable(const EAssetCategory assetType);
        const VirtualPathGuidTable& GetVirtualPathGuidTable(const EAssetCategory assetType) const;

        template <typename T>
        AssetDescMap<T>& GetDescMap()
        {
            return static_cast<AssetDescMap<T>&>(GetDescMap(AssetCategoryOf<T>));
        }

        template <typename T>
        const AssetDescMap<T>& GetDescMap() const
        {
            return static_cast<const AssetDescMap<T>&>(GetDescMap(AssetCategoryOf<T>));
        }

        TypelessAssetDescMap& GetDescMap(const EAssetCategory assetType);
        const TypelessAssetDescMap& GetDescMap(const EAssetCategory assetType) const;

        [[nodiscard]] bool ContainsUnsafe(const Guid& guid) const;
        [[nodiscard]] bool ContainsUnsafe(const EAssetCategory assetType, const std::string_view virtualPath) const;

        [[nodiscard]] Guid GetGuidUnsafe(const EAssetCategory assetType, const std::string_view virtualPath) const;
        [[nodiscard]] AssetInfo GetAssetInfoUnsafe(const Guid& guid) const;

        template <typename T>
        [[nodiscard]] typename T::LoadDesc GetLoadDescUnsafe(const Guid& guid) const
        {
            IG_CHECK(ContainsUnsafe(guid));
            const AssetDescMap<T>& assetDescTable{GetDescMap<T>()};
            return assetDescTable.GetDesc(guid).LoadDescriptor;
        }

        template <typename T>
        void UpdateLoadDescUnsafe(const Guid& guid, const typename T::LoadDesc& loadDesc)
        {
            IG_CHECK(ContainsUnsafe(guid));
            AssetDescMap<T>& assetDescTable{GetDescMap<T>()};
            assetDescTable.Update(guid, loadDesc);
        }

        template <typename T>
        [[nodiscard]] typename T::Desc GetDescUnsafe(const Guid& guid) const
        {
            IG_CHECK(ContainsUnsafe(guid));
            const AssetDescMap<T>& assetDescTable{GetDescMap<T>()};
            return assetDescTable.GetDesc(guid);
        }

        void ReflectExpiredToFilesUnsafe();
        void ReflectRemainedToFilesUnsafe();
        static void CleanupOrphanFiles();

    private:
        mutable SharedMutex mutex;
        Vector<std::pair<EAssetCategory, VirtualPathGuidTable>> virtualPathGuidTables;
        Vector<std::pair<EAssetCategory, Ptr<TypelessAssetDescMap>>> guidDescTables;
        UnorderedMap<Guid, AssetInfo> expiredAssetInfos;
    };
} // namespace ig::details
