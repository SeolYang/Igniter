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
            const U64 HandleHash{IG_NUMERIC_MAX_OF(HandleHash)};
            const U32 RefCount{};
        };

    public:
        virtual ~TypelessAssetCache() = default;

        virtual EAssetCategory GetAssetType() const = 0;
        virtual void Invalidate(const Guid& guid) = 0;
        virtual [[nodiscard]] bool IsCached(const Guid& guid) const = 0;
        virtual [[nodiscard]] Vector<Snapshot> TakeSnapshots() const = 0;
        [[nodiscard]] virtual Snapshot TakeSnapshot(const Guid& guid) const = 0;
    };

    template <typename T>
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

        void Cache(const Guid& guid, T asset)
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

        [[nodiscard]] Handle<T> Load(const Guid& guid, const bool bShouldIncreaseRefCounter = true)
        {
            ReadWriteLock rwLock{mutex};
            return LoadUnsafe(guid, bShouldIncreaseRefCounter);
        }

        void Clone(const Guid& guid, const U32 numClones = 1)
        {
            IG_CHECK(numClones > 0);
            IG_CHECK(guid.isValid());

            ReadWriteLock rwLock{mutex};
            IG_CHECK(cachedAssets.contains(guid));
            IG_CHECK(refCounterTable.contains(guid));

            refCounterTable[guid] += numClones;
        }

        [[nodiscard]] bool IsCached(const Guid& guid) const override
        {
            ReadOnlyLock lock{mutex};
            return IsCachedUnsafe(guid);
        }

        [[nodiscard]] T* Lookup(const Handle<T> handle)
        {
            ReadOnlyLock lock{mutex};
            return registry.Lookup(handle);
        }

        [[nodiscard]] const T* Lookup(const Handle<T> handle) const
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
            const U32 refCount{--refCounterTable[guid]};
            if (refCount == 0 && assetInfo.GetScope() == EAssetScope::Managed)
            {
                InvalidateUnsafe(guid);
            }
        }

        // #sy_todo Unload with Handle?

        [[nodiscard]] Vector<Snapshot> TakeSnapshots() const override
        {
            ReadOnlyLock lock{mutex};
            Vector<Snapshot> refCounterSnapshots{};
            refCounterSnapshots.reserve(refCounterTable.size());
            for (const auto& guidRefCounter : refCounterTable)
            {
                refCounterSnapshots.emplace_back(Snapshot{.HandleHash = cachedAssets.at(guidRefCounter.first).GetHash(), .RefCount = guidRefCounter.second});
            }

            return refCounterSnapshots;
        }

        [[nodiscard]] Snapshot TakeSnapshot(const Guid& guid) const override
        {
            ReadOnlyLock lock{mutex};
            const auto refCounterItr = refCounterTable.find(guid);
            return refCounterItr != refCounterTable.cend() ? Snapshot{.HandleHash = cachedAssets.at(guid).GetHash(), .RefCount = refCounterItr->second} : Snapshot{};
        }

    private:
        [[nodiscard]] bool IsCachedUnsafe(const Guid& guid) const
        {
            IG_CHECK(guid.isValid());
            return cachedAssets.contains(guid) && refCounterTable.contains(guid);
        }

        [[nodiscard]] Handle<T> LoadUnsafe(const Guid& guid, const bool bShouldIncreaseRefCounter = true)
        {
            IG_CHECK(guid.isValid());
            IG_CHECK(cachedAssets.contains(guid));
            IG_CHECK(refCounterTable.contains(guid));

            if (bShouldIncreaseRefCounter)
            {
                ++refCounterTable[guid];
            }

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
        HandleStorage<T> registry;
        UnorderedMap<Guid, Handle<T>> cachedAssets{};
        UnorderedMap<Guid, U32> refCounterTable{};
    };
} // namespace ig::details
