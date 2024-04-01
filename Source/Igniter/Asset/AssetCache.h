#pragma once
#include <Igniter.h>
#include <Core/String.h>
#include <Core/Handle.h>
#include <Asset/Common.h>
#include <Asset/Utils.h>

namespace ig::details
{
    class TypelessAssetCache
    {
    public:
        virtual ~TypelessAssetCache() = default;

        virtual EAssetType GetAssetType() const = 0;
        virtual void Invalidate(const xg::Guid guid) = 0;
        virtual bool IsCached(const xg::Guid guid) const = 0;
    };

    template <Asset T>
    class AssetCache final : public TypelessAssetCache
    {
        friend class UniqueRefHandle<T, class AssetCache<T>*>;

    public:
        AssetCache(HandleManager& handleManager) : handleManager(handleManager)
        {
        }

        AssetCache(const AssetCache&) = delete;
        AssetCache(AssetCache&&) noexcept = delete;
        ~AssetCache() = default;

        AssetCache& operator=(const AssetCache&) = delete;
        AssetCache& operator=(AssetCache&&) noexcept = delete;

        EAssetType GetAssetType() const override { return AssetType; }

        [[nodiscard]] CachedAsset<T> Cache(const xg::Guid guid, T&& asset)
        {
            IG_CHECK(guid.isValid());

            ReadWriteLock rwLock{ mutex };
            IG_CHECK(!cachedAssets.contains(guid));
            IG_CHECK(!refCounterTable.contains(guid));
            cachedAssets[guid] = Handle<T>{ handleManager, std::move(asset) };
            refCounterTable[guid] = 0;
            return LoadUnsafe(guid);
        }

        void Invalidate(const xg::Guid guid) override
        {
            ReadWriteLock rwLock{ mutex };
            InvalidateUnsafe(guid);
        }

        [[nodiscard]] CachedAsset<T> Load(const xg::Guid guid)
        {
            ReadWriteLock rwLock{ mutex };
            return LoadUnsafe(guid);
        }

        [[nodiscard]] bool IsCached(const xg::Guid guid) const
        {
            ReadOnlyLock lock{ mutex };
            return IsCachedUnsafe(guid);
        }

    private:
        [[nodiscard]] bool IsCachedUnsafe(const xg::Guid guid) const
        {
            IG_CHECK(guid.isValid());
            return cachedAssets.contains(guid) && refCounterTable.contains(guid);
        }

        [[nodiscard]] CachedAsset<T> LoadUnsafe(const xg::Guid guid)
        {
            IG_CHECK(guid.isValid());
            IG_CHECK(cachedAssets.contains(guid));
            IG_CHECK(refCounterTable.contains(guid));

            ++refCounterTable[guid];
            return cachedAssets[guid].MakeUniqueRef(this);
        }

        void InvalidateUnsafe(const xg::Guid guid)
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
            const AssetDesc<T> snapshot{ asset->GetSnapshot() };
            const AssetInfo& assetInfo{ snapshot.Info };
            IG_CHECK(assetInfo.IsValid());

            /* #sy_wip 영속 에셋 인스턴스에 대한 처리 필요 */
            ReadWriteLock rwLock{ mutex };
            IG_CHECK(cachedAssets.contains(assetInfo.Guid));
            IG_CHECK(refCounterTable.contains(assetInfo.Guid));
            IG_CHECK(refCounterTable[assetInfo.Guid] > 0);
            const uint32_t refCount{ --refCounterTable[assetInfo.Guid] };
            if (refCount == 0)
            {
                InvalidateUnsafe(assetInfo.Guid);
            }
        }

    public:
        constexpr static EAssetType AssetType = AssetTypeOf_v<T>;

    private:
        HandleManager& handleManager;

        /* #sy_wip 캐시된 에셋 맵에 대한 스레드 세이프 보장 할 것. */
        mutable SharedMutex mutex;
        robin_hood::unordered_map<xg::Guid, Handle<T>> cachedAssets{};
        robin_hood::unordered_map<xg::Guid, uint32_t> refCounterTable{};
    };
} // namespace ig::details