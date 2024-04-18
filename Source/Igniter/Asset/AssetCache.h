#pragma once
#include <Igniter.h>
#include <Core/String.h>
#include <Core/Handle.h>
#include <Asset/Common.h>

namespace ig::details
{
    class TypelessAssetCache
    {
    public:
        struct Snapshot
        {
            const Guid     Guid{};
            const uint32_t RefCount{};
        };

    public:
        virtual ~TypelessAssetCache() = default;

        virtual EAssetType                          GetAssetType() const = 0;
        virtual void                                Invalidate(const Guid& guid) = 0;
        virtual [[nodiscard]] bool                  IsCached(const Guid& guid) const = 0;
        virtual [[nodiscard]] std::vector<Snapshot> TakeSnapshots() const = 0;
    };

    template <Asset T>
    class AssetCache final : public TypelessAssetCache
    {
        friend class UniqueRefHandle<T, class AssetCache<T>*>;

    public:
        AssetCache(HandleManager& handleManager)
            : handleManager(handleManager)
        {
        }

        AssetCache(const AssetCache&)     = delete;
        AssetCache(AssetCache&&) noexcept = delete;
        ~AssetCache()                     = default;

        AssetCache& operator=(const AssetCache&)     = delete;
        AssetCache& operator=(AssetCache&&) noexcept = delete;

        EAssetType GetAssetType() const override { return AssetType; }

        void Cache(const Guid& guid, T&& asset)
        {
            IG_CHECK(guid.isValid());

            ReadWriteLock rwLock{mutex};
            IG_CHECK(!cachedAssets.contains(guid));
            IG_CHECK(!refCounterTable.contains(guid));
            cachedAssets[guid]    = Handle<T>{handleManager, std::move(asset)};
            refCounterTable[guid] = 0;
        }

        void Invalidate(const Guid& guid) override
        {
            ReadWriteLock rwLock{mutex};
            InvalidateUnsafe(guid);
        }

        [[nodiscard]] CachedAsset<T> Load(const Guid& guid)
        {
            ReadWriteLock rwLock{mutex};
            return LoadUnsafe(guid);
        }

        [[nodiscard]] bool IsCached(const Guid& guid) const
        {
            ReadOnlyLock lock{mutex};
            return IsCachedUnsafe(guid);
        }

        [[nodiscard]] std::vector<Snapshot> TakeSnapshots() const override
        {
            ReadOnlyLock          lock{mutex};
            std::vector<Snapshot> refCounterSnapshots{};
            refCounterSnapshots.reserve(refCounterTable.size());
            for (const auto& guidRefCounter : refCounterTable)
            {
                refCounterSnapshots.emplace_back(Snapshot{
                    .Guid = guidRefCounter.first, .RefCount = guidRefCounter.second
                });
            }

            return refCounterSnapshots;
        }

    private:
        [[nodiscard]] bool IsCachedUnsafe(const Guid& guid) const
        {
            IG_CHECK(guid.isValid());
            return cachedAssets.contains(guid) && refCounterTable.contains(guid);
        }

        [[nodiscard]] CachedAsset<T> LoadUnsafe(const Guid& guid)
        {
            IG_CHECK(guid.isValid());
            IG_CHECK(cachedAssets.contains(guid));
            IG_CHECK(refCounterTable.contains(guid));

            ++refCounterTable[guid];
            return cachedAssets[guid].MakeUniqueRef(this);
        }

        void InvalidateUnsafe(const Guid& guid)
        {
            IG_CHECK(guid.isValid());
            IG_CHECK(cachedAssets.contains(guid));
            IG_CHECK(refCounterTable.contains(guid));

            cachedAssets.erase(guid);
            refCounterTable.erase(guid);
        }

        void operator()(T* asset)
        {
            IG_CHECK(asset != nullptr);
            const AssetDesc<T> snapshot{asset->GetSnapshot()};
            const AssetInfo&   assetInfo{snapshot.Info};
            IG_CHECK(assetInfo.IsValid());

            const Guid& guid{assetInfo.GetGuid()};

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

    public:
        constexpr static EAssetType AssetType = AssetTypeOf<T>;

    private:
        HandleManager& handleManager;

        mutable SharedMutex           mutex;
        UnorderedMap<Guid, Handle<T>> cachedAssets{};
        UnorderedMap<Guid, uint32_t>  refCounterTable{};
    };
} // namespace ig::details
