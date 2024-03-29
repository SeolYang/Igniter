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
        virtual void Uncache(const xg::Guid guid) = 0;
        virtual bool IsCached(const xg::Guid guid) = 0;
    };

    template <Asset T>
    class AssetCache final : public TypelessAssetCache
    {
    public:
        AssetCache() = default;
        AssetCache(const AssetCache&) = delete;
        AssetCache(AssetCache&&) noexcept = delete;
        ~AssetCache() = default;

        AssetCache& operator=(const AssetCache&) = delete;
        AssetCache& operator=(AssetCache&&) noexcept = delete;

        EAssetType GetAssetType() const override { return AssetType; }

        void Cache(const xg::Guid guid, T)
        {
            IG_CHECK(guid.isValid());
            IG_CHECK(!cachedAssets.contains(guid));
            IG_UNIMPLEMENTED();
        }

        void Uncache(const xg::Guid) override
        {
            IG_UNIMPLEMENTED();
        }

        [[nodiscard]] RefHandle<T> GetCachedAsset(const xg::Guid guid)
        {
            if (!IsCached(guid))
            {
                return {};
            }

            return cachedAssets[guid].MakeRef();
        }

        bool IsCached(const xg::Guid guid) { return cachedAssets.contains(guid); }

    public:
        constexpr static EAssetType AssetType = AssetTypeOf_v<T>;

    private:
        robin_hood::unordered_map<xg::Guid, Handle<T>> cachedAssets{};
    };
} // namespace ig::details