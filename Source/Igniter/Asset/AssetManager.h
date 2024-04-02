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

namespace ig
{
    class TextureImporter;
    class TextureLoader;
    class StaticMeshImporter;
    class StaticMeshLoader;
    class MaterialLoader;
    class AssetManager final
    {
    private:
        using VirtualPathGuidTable = robin_hood::unordered_map<String, xg::Guid>;

    public:
        using ModifiedEvent = Event<String, std::reference_wrapper<const AssetManager>>;

        struct Snapshot
        {
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

        xg::Guid ImportTexture(const String resPath, const TextureImportDesc& desc);
        [[nodiscard]] CachedAsset<Texture> LoadTexture(const xg::Guid guid);
        [[nodiscard]] CachedAsset<Texture> LoadTexture(const String virtualPath);

        std::vector<xg::Guid> ImportStaticMesh(const String resPath, const StaticMeshImportDesc& desc);
        [[nodiscard]] CachedAsset<StaticMesh> LoadStaticMesh(const xg::Guid guid);
        [[nodiscard]] CachedAsset<StaticMesh> LoadStaticMesh(const String virtualPath);

        xg::Guid CreateMaterial(MaterialCreateDesc desc);
        [[nodiscard]] CachedAsset<Material> LoadMaterial(const xg::Guid guid);
        [[nodiscard]] CachedAsset<Material> LoadMaterial(const String virtualPath);

        void Delete(const xg::Guid guid);
        void Delete(const EAssetType assetType, const String virtualPath);

        /* #sy_todo Reload ASSET! */

        void SaveAllChanges();

        [[nodiscard]] AssetInfo GetAssetInfo(const xg::Guid guid) const;
        [[nodiscard]] ModifiedEvent& GetModifiedEvent() { return assetModifiedEvent; }

        /* #sy_wip Gathering All Asset Status & Informations */
        std::vector<Snapshot> TakeSnapshots();

    private:
        template <Asset T>
        details::AssetCache<T>& GetCache()
        {
            return static_cast<details::AssetCache<T>&>(GetTypelessCache(AssetTypeOf_v<T>));
        }

        details::TypelessAssetCache& GetTypelessCache(const EAssetType assetType);

        void InitAssetCaches(HandleManager& handleManager);

        template <Asset T, typename AssetLoader>
        [[nodiscard]] CachedAsset<T> LoadInternal(const xg::Guid guid, AssetLoader& loader)
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
                    IG_LOG(AssetManager, Info, "{} asset {}({}) cached.",
                           AssetTypeOf_v<T>,
                           desc.Info.GetVirtualPath(), guid);
                    assetModifiedEvent.Notify(*this);
                    return assetCache.Cache(guid, result.Take());
                }

                IG_LOG(AssetManager, Error, "Failed({}) to load {} asset {}({}).",
                       AssetTypeOf_v<T>, result.GetStatus(),
                       desc.Info.GetVirtualPath(), guid);
                return CachedAsset<T>{};
            }

            IG_LOG(AssetManager, Info, "Cache Hit! {} asset {} loaded.", AssetTypeOf_v<T>, guid);
            assetModifiedEvent.Notify(*this);
            return assetCache.Load(guid);
        }

        void DeleteInternal(const EAssetType assetType, const xg::Guid guid);

        [[nodiscard]] UniqueLock RequestAssetLock(const xg::Guid guid);

        /* #sy_wip RegisterEngineAsset() */

    private:
        Ptr<details::AssetMonitor> assetMonitor;
        std::vector<Ptr<details::TypelessAssetCache>> assetCaches;

        Mutex assetMutexTableMutex;
        robin_hood::unordered_map<xg::Guid, Mutex> assetMutexTable;

        Ptr<TextureImporter> textureImporter;
        Ptr<TextureLoader> textureLoader;

        Ptr<StaticMeshImporter> staticMeshImporter;
        Ptr<StaticMeshLoader> staticMeshLoader;

        Ptr<MaterialLoader> materialLoader;

        ModifiedEvent assetModifiedEvent;
    };
} // namespace ig