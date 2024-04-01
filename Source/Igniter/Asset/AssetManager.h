#pragma once
#include <Core/Result.h>
#include <Core/Log.h>
#include <Asset/Common.h>
#include <Asset/AssetMonitor.h>
#include <Asset/AssetCache.h>
#include <Asset/Texture.h>
#include <Asset/StaticMesh.h>

IG_DEFINE_LOG_CATEGORY(AssetManager);

namespace ig
{
    class TextureImporter;
    class TextureLoader;
    class StaticMeshImporter;
    class StaticMeshLoader;
    class AssetManager final
    {
        using VirtualPathGuidTable = robin_hood::unordered_map<String, xg::Guid>;

    public:
        AssetManager(HandleManager& handleManager, RenderDevice& renderDevice, GpuUploader& gpuUploader, GpuViewManager& gpuViewManager);
        AssetManager(const AssetManager&) = delete;
        AssetManager(AssetManager&&) noexcept = delete;
        ~AssetManager();

        AssetManager& operator=(const AssetManager&) = delete;
        AssetManager& operator=(AssetManager&&) noexcept = delete;

        xg::Guid ImportTexture(const String resPath, const TextureImportDesc& config);
        [[nodiscard]] CachedAsset<Texture> LoadTexture(const xg::Guid guid);
        [[nodiscard]] CachedAsset<Texture> LoadTexture(const String virtualPath);

        std::vector<xg::Guid> ImportStaticMesh(const String resPath, const StaticMeshImportDesc& desc);
        [[nodiscard]] CachedAsset<StaticMesh> LoadStaticMesh(const xg::Guid guid);
        [[nodiscard]] CachedAsset<StaticMesh> LoadStaticMesh(const String virtualPath);

        void Delete(const xg::Guid guid);
        void Delete(const EAssetType assetType, const String virtualPath);

        [[nodiscard]] AssetInfo GetAssetInfo(const xg::Guid guid) const;

        void SaveAllChanges();

    private:
        template <Asset T>
        details::AssetCache<T>& GetCache()
        {
            return static_cast<details::AssetCache<T>&>(GetTypelessCache(AssetTypeOf_v<T>));
        }

        details::TypelessAssetCache& GetTypelessCache(const EAssetType assetType);

        void InitAssetCaches(HandleManager& handleManager);

        void DeleteInternal(const EAssetType assetType, const xg::Guid guid);
        
        template <Asset T, typename AssetLoader>
        [[nodiscard]] CachedAsset<T> LoadInternal(const xg::Guid guid, AssetLoader& loader)
        {
            if (!assetMonitor->Contains(guid))
            {
                IG_LOG(AssetManager, Error, "{} asset \"{}\" is invisible to asset manager.", AssetTypeOf_v<T>, guid);
                return CachedAsset<T>{};
            }

            details::AssetCache<T>& assetCache{ GetCache<T>() };
            if (!assetCache.IsCached(guid))
            {
                const T::Desc desc{ assetMonitor->GetDesc<T>(guid) };
                IG_CHECK(desc.Info.Guid == guid);
                auto result{ loader.Load(desc) };
                if (result.HasOwnership())
                {
                    IG_LOG(AssetManager, Info, "{} asset {}({}) cached.", AssetTypeOf_v<T>, desc.Info.VirtualPath, desc.Info.Guid);
                    return assetCache.Cache(guid, result.Take());
                }

                IG_LOG(AssetManager, Error, "Failed({}) to load {} asset {}({}).", AssetTypeOf_v<T>, result.GetStatus(), desc.Info.VirtualPath, desc.Info.Guid);
                return CachedAsset<T>{};
            }

            IG_LOG(AssetManager, Info, "Cache Hit! {} asset {} loaded.", AssetTypeOf_v<T>, guid);
            return assetCache.Load(guid);
        }

    private:
        Ptr<details::AssetMonitor> assetMonitor;
        std::vector<Ptr<details::TypelessAssetCache>> assetCaches;

        Ptr<TextureImporter> textureImporter;
        Ptr<TextureLoader> textureLoader;

        Ptr<StaticMeshImporter> staticMeshImporter;
        Ptr<StaticMeshLoader> staticMeshLoader;
    };

} // namespace ig