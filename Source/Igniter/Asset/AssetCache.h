#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/String.h"
#include "Igniter/Core/Handle.h"
#include "Igniter/Core/HandleStorage.h"
#include "Igniter/Asset/Common.h"

namespace ig::details
{
    class TypelessAssetCache
    {
    public:
        struct Snapshot
        {
            const Guid Guid{};
            const uint32_t RefCount{};
        };

    public:
        virtual ~TypelessAssetCache() = default;

        virtual EAssetCategory GetAssetType() const = 0;
        virtual void Invalidate(const Guid& guid) = 0;
        virtual [[nodiscard]] bool IsCached(const Guid& guid) const = 0;
        virtual [[nodiscard]] std::vector<Snapshot> TakeSnapshots() const = 0;
    };

    template <Asset T>
    class AssetCache final : public TypelessAssetCache
    {
    public:
        AssetCache() = default;
        AssetCache(const AssetCache&) = delete;
        AssetCache(AssetCache&&) noexcept = delete;
        ~AssetCache() override = default;

        AssetCache& operator=(const AssetCache&) = delete;
        AssetCache& operator=(AssetCache&&) noexcept = delete;

        EAssetCategory GetAssetType() const override { return AssetType; }

        void Cache(const Guid& guid, T&& asset)
        {
            IG_CHECK(guid.isValid());

            ReadWriteLock rwLock{mutex};
            IG_CHECK(!cachedAssets.contains(guid));
            IG_CHECK(!refCounterTable.contains(guid));
            cachedAssets[guid] = registry.Create(std::move(asset));
            refCounterTable[guid] = 0;
        }

        void Invalidate(const Guid& guid) override
        {
            ReadWriteLock rwLock{mutex};
            InvalidateUnsafe(guid);
        }

        [[nodiscard]] ManagedAsset<T> Load(const Guid& guid)
        {
            ReadWriteLock rwLock{mutex};
            return LoadUnsafe(guid);
        }

        [[nodiscard]] bool IsCached(const Guid& guid) const override
        {
            ReadOnlyLock lock{mutex};
            return IsCachedUnsafe(guid);
        }

        [[nodiscard]] T* Lookup(const ManagedAsset<T> handle)
        {
            ReadOnlyLock lock{mutex};
            return registry.Lookup(handle);
        }

        [[nodiscard]] const T* Lookup(const ManagedAsset<T> handle) const
        {
            ReadOnlyLock lock{mutex};
            return registry.Lookup(handle);
        }

        void Unload(const AssetInfo& assetInfo)
        {
            const Guid& guid = assetInfo.GetGuid();
            ReadWriteLock rwLock{mutex};
            IG_CHECK(cachedAssets.contains(guid));
            IG_CHECK(refCounterTable.contains(guid));
            IG_CHECK(refCounterTable[guid] > 0);
            const uint32_t refCount{--refCounterTable[guid]};
            if (refCount == 0 && assetInfo.GetScope() == EAssetScope::Managed)
            {
                InvalidateUnsafe(guid);
            }
        }

        // #sy_todo Unload with Handle?

        [[nodiscard]] std::vector<Snapshot> TakeSnapshots() const override
        {
            ReadOnlyLock lock{mutex};
            std::vector<Snapshot> refCounterSnapshots{};
            refCounterSnapshots.reserve(refCounterTable.size());
            for (const auto& guidRefCounter : refCounterTable)
            {
                refCounterSnapshots.emplace_back(Snapshot{.Guid = guidRefCounter.first, .RefCount = guidRefCounter.second});
            }

            return refCounterSnapshots;
        }

    private:
        [[nodiscard]] bool IsCachedUnsafe(const Guid& guid) const
        {
            IG_CHECK(guid.isValid());
            return cachedAssets.contains(guid) && refCounterTable.contains(guid);
        }

        [[nodiscard]] ManagedAsset<T> LoadUnsafe(const Guid& guid)
        {
            IG_CHECK(guid.isValid());
            IG_CHECK(cachedAssets.contains(guid));
            IG_CHECK(refCounterTable.contains(guid));

            ++refCounterTable[guid];
            return cachedAssets[guid];
        }

        void InvalidateUnsafe(const Guid& guid)
        {
            IG_CHECK(guid.isValid());
            IG_CHECK(cachedAssets.contains(guid));
            IG_CHECK(refCounterTable.contains(guid));

            registry.Destroy(cachedAssets[guid]);
            cachedAssets.erase(guid);
            refCounterTable.erase(guid);
        }

    public:
        constexpr static EAssetCategory AssetType = AssetCategoryOf<T>;

    private:
        mutable SharedMutex mutex;
        HandleStorage<T, class AssetManager> registry;
        UnorderedMap<Guid, ManagedAsset<T>> cachedAssets{};
        UnorderedMap<Guid, uint32_t> refCounterTable{};
    };
}    // namespace ig::details
