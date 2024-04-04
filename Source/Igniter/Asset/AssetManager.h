#pragma once
#include <Core/Result.h>
#include <Core/Log.h>
#include <Core/Event.h>
#include <Asset/Common.h>
#include <Asset/AssetMonitor.h>
#include <Asset/AssetCache.h>
#include <Asset/Texture.h>
#include <Asset/StaticMesh.h>
#include <Asset/Material.h>

IG_DEFINE_LOG_CATEGORY(AssetManager);

namespace ig::details
{
    enum class EDefaultDummyStatus
    {
        Success,
    };
}

namespace ig
{
    class TextureImporter;
    class TextureLoader;
    class StaticMeshImporter;
    class StaticMeshLoader;
    class MaterialImporter;
    class MaterialLoader;
    class AssetManager final
    {
    private:
        using VirtualPathGuidTable = UnorderedMap<String, Guid>;

    public:
        using ModifiedEvent = Event<String, std::reference_wrapper<const AssetManager>>;

        struct Snapshot
        {
        public:
            bool IsCached() const
            {
                return RefCount > 0 ||
                       Info.GetPersistency() == EAssetPersistency::Persistent ||
                       Info.GetPersistency() == EAssetPersistency::Engine;
            }

        public:
            AssetInfo Info{};
            uint32_t RefCount{};
        };

    public:
        AssetManager(HandleManager& handleManager, RenderDevice& renderDevice, GpuUploader& gpuUploader, GpuViewManager& gpuViewManager);
        AssetManager(const AssetManager&) = delete;
        AssetManager(AssetManager&&) noexcept = delete;
        ~AssetManager();

        AssetManager& operator=(const AssetManager&) = delete;
        AssetManager& operator=(AssetManager&&) noexcept = delete;

        Guid ImportTexture(const String resPath, const TextureImportDesc& desc);
        [[nodiscard]] CachedAsset<Texture> LoadTexture(const Guid guid);
        [[nodiscard]] CachedAsset<Texture> LoadTexture(const String virtualPath);

        std::vector<Guid> ImportStaticMesh(const String resPath, const StaticMeshImportDesc& desc);
        [[nodiscard]] CachedAsset<StaticMesh> LoadStaticMesh(const Guid guid);
        [[nodiscard]] CachedAsset<StaticMesh> LoadStaticMesh(const String virtualPath);

        Guid CreateMaterial(const String virtualPath, const MaterialCreateDesc desc);
        [[nodiscard]] CachedAsset<Material> LoadMaterial(const Guid guid);
        [[nodiscard]] CachedAsset<Material> LoadMaterial(const String virtualPath);

        void Delete(const Guid guid);
        void Delete(const EAssetType assetType, const String virtualPath);

        /* #sy_todo Reload ASSET! */

        void SaveAllChanges();

        [[nodiscard]] AssetInfo GetAssetInfo(const Guid guid) const;
        [[nodiscard]] ModifiedEvent& GetModifiedEvent() { return assetModifiedEvent; }

        [[nodiscard]] std::vector<Snapshot> TakeSnapshots() const;

    private:
        template <Asset T>
        details::AssetCache<T>& GetCache()
        {
            return static_cast<details::AssetCache<T>&>(GetTypelessCache(AssetTypeOf_v<T>));
        }

        details::TypelessAssetCache& GetTypelessCache(const EAssetType assetType);

        void InitAssetCaches(HandleManager& handleManager);

        void InitEngineDefaultAssets();

        template <Asset T, typename AssetLoader>
        [[nodiscard]] CachedAsset<T> LoadInternal(const Guid guid, AssetLoader& loader)
        {
            if (!assetMonitor->Contains(guid))
            {
                IG_LOG(AssetManager, Error, "{} asset \"{}\" is invisible to asset manager.", AssetTypeOf_v<T>, guid);
                return CachedAsset<T>{};
            }

            UniqueLock assetLock{ RequestAssetLock(guid) };
            details::AssetCache<T>& assetCache{ GetCache<T>() };
            if (!assetCache.IsCached(guid))
            {
                const T::Desc desc{ assetMonitor->GetDesc<T>(guid) };
                IG_CHECK(desc.Info.GetGuid() == guid);
                auto result{ loader.Load(desc) };
                if (result.HasOwnership())
                {
                    IG_LOG(AssetManager, Info, "{} asset {} ({}) cached.",
                           AssetTypeOf_v<T>,
                           desc.Info.GetVirtualPath(), guid);
                    assetModifiedEvent.Notify(*this);
                    return assetCache.Cache(guid, result.Take());
                }

                IG_LOG(AssetManager, Error, "Failed({}) to load {} asset {} ({}).",
                       AssetTypeOf_v<T>, result.GetStatus(),
                       desc.Info.GetVirtualPath(), guid);
                return CachedAsset<T>{};
            }

            IG_LOG(AssetManager, Info, "Cache Hit! {} asset {} loaded.", AssetTypeOf_v<T>, guid);
            assetModifiedEvent.Notify(*this);
            return assetCache.Load(guid);
        }

        template <Asset T, ResultStatus ImportStatus>
        std::optional<Guid> ImportInternal(String resPath, Result<typename T::Desc, ImportStatus>& result)
        {
            constexpr auto AssetType{ AssetTypeOf_v<T> };
            if (!result.HasOwnership())
            {
                IG_LOG(AssetManager, Error, "{}: Failed({}) to import  \"{}\".",
                       AssetType,
                       result.GetStatus(),
                       resPath);

                return std::nullopt;
            }

            const T::Desc desc{ result.Take() };
            const AssetInfo& assetInfo{ desc.Info };
            IG_CHECK(assetInfo.IsValid());
            IG_CHECK(assetInfo.GetType() == AssetType);

            const String virtualPath{ assetInfo.GetVirtualPath() };
            IG_CHECK(IsValidVirtualPath(virtualPath));

            if (assetMonitor->Contains(AssetType, virtualPath))
            {
                const AssetInfo currentAssetInfo{ assetMonitor->GetAssetInfo(AssetType, virtualPath) };
                IG_CHECK(currentAssetInfo.IsValid());
                if (currentAssetInfo.GetPersistency() == EAssetPersistency::Engine)
                {
                    IG_LOG(AssetManager, Error, "{}: Failed to import \"{}\". Given virtual path {} was reserved by engine.",
                           AssetType,
                           resPath,
                           virtualPath);

                    return std::nullopt;
                }

                Delete(AssetType, virtualPath);
            }
            assetMonitor->Create<T>(assetInfo, desc.LoadDescriptor);

            IG_LOG(AssetManager, Info, "{}: \"{}\" imported as {} ({}).",
                   AssetType,
                   resPath,
                   virtualPath,
                   assetInfo.GetGuid());
            return assetInfo.GetGuid();
        }

        void DeleteInternal(const EAssetType assetType, const Guid guid);

        [[nodiscard]] UniqueLock RequestAssetLock(const Guid guid);

        template <Asset T, ResultStatus Status>
        void RegisterEngineDefault(const String requiredVirtualPath, Result<T, Status> assetResult)
        {
            if (!assetResult.HasOwnership())
            {
                IG_LOG(AssetManager, Error, "{}: Failed({}) to create engine default asset {}.",
                       AssetTypeOf_v<T>, assetResult.GetStatus(), requiredVirtualPath);
                return;
            }

            T asset{ assetResult.Take() };
            const typename T::Desc& desc{ asset.GetSnapshot() };
            const AssetInfo& assetInfo{ desc.Info };
            IG_CHECK(assetInfo.IsValid());
            IG_CHECK(assetInfo.GetType() == AssetTypeOf_v<T>);
            IG_CHECK(assetInfo.GetPersistency() == EAssetPersistency::Engine);

            /* #sy_note GUID를 고정으로 할지 말지.. 일단 Virtual Path는 고정적이고, Import 되고난 이후엔 Virtual Path로 접근 가능하니 유동적으로 */
            Result<typename T::Desc, details::EDefaultDummyStatus> dummyResult{ MakeSuccess<typename T::Desc, details::EDefaultDummyStatus>(desc) };
            IG_CHECK(dummyResult.HasOwnership());
            std::optional<Guid> guidOpt{ ImportInternal<T>(assetInfo.GetVirtualPath(), dummyResult) };
            IG_CHECK(guidOpt);

            const Guid guid{ *guidOpt };
            IG_CHECK(guid.isValid());
            UniqueLock assetLock{ RequestAssetLock(guid) };
            details::AssetCache<T>& assetCache{ GetCache<T>() };
            IG_CHECK(!assetCache.IsCached(guid));
            [[maybe_unused]] CachedAsset<T> cachedAsset{ assetCache.Cache(guid, std::move(asset)) };
            IG_CHECK(cachedAsset);
        }

    private:
        Ptr<details::AssetMonitor> assetMonitor;
        std::vector<Ptr<details::TypelessAssetCache>> assetCaches;

        Mutex assetMutexTableMutex;
        StableUnorderedMap<Guid, Mutex> assetMutexTable;

        Ptr<TextureImporter> textureImporter;
        Ptr<TextureLoader> textureLoader;

        Ptr<StaticMeshImporter> staticMeshImporter;
        Ptr<StaticMeshLoader> staticMeshLoader;

        Ptr <MaterialImporter> materialImporter;
        Ptr<MaterialLoader> materialLoader;

        ModifiedEvent assetModifiedEvent;
    };
} // namespace ig