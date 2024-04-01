#pragma once
#include <Core/Result.h>
#include <Asset/Common.h>
#include <Asset/Texture.h>
#include <Asset/StaticMesh.h>

namespace ig::details
{
    class AssetMonitor;
    class TypelessAssetCache;
    template <Asset T>
    class AssetCache;
} // namespace ig::details

namespace ig
{
    struct Texture;
    class TextureImporter;
    class TextureLoader;
    struct StaticMesh;
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
        CachedAsset<Texture> LoadTexture(const xg::Guid guid);
        CachedAsset<Texture> LoadTexture(const String virtualPath);

        std::vector<xg::Guid> ImportStaticMesh(const String resPath, const StaticMeshImportDesc& desc);
        CachedAsset<StaticMesh> LoadStaticMesh(const xg::Guid guid);
        CachedAsset<StaticMesh> LoadStaticMesh(const String virtualPath);

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

    private:
        Ptr<details::AssetMonitor> assetMonitor;
        std::vector<Ptr<details::TypelessAssetCache>> assetCaches;

        Ptr<TextureImporter> textureImporter;
        Ptr<TextureLoader> textureLoader;

        Ptr<StaticMeshImporter> staticMeshImporter;
        Ptr<StaticMeshLoader> staticMeshLoader;
    };

} // namespace ig