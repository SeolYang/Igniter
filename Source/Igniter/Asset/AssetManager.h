#pragma once
#include <Core/Handle.h>
#include <Core/Result.h>
#include <Asset/Common.h>

namespace ig::details
{
    class AssetMonitor;
    class TypelessAssetCache;
    template <Asset T>
    class AssetCache;
} // namespace ig::details

namespace ig
{
    template <typename AssetType>
    using AssetRefHandle = RefHandle<AssetType, class AssetManager*>;

    struct Texture;
    struct TextureImportDesc;
    class TextureImporter;
    struct StaticMeshImportDesc;
    class AssetManager final
    {
        friend class RefHandle<Texture, AssetManager*>;
        friend class Handle<Texture, AssetManager*>;
        using VirtualPathGuidTable = robin_hood::unordered_map<String, xg::Guid>;

    public:
        AssetManager();
        AssetManager(const AssetManager&) = delete;
        AssetManager(AssetManager&&) noexcept = delete;
        ~AssetManager();

        AssetManager& operator=(const AssetManager&) = delete;
        AssetManager& operator=(AssetManager&&) noexcept = delete;

        xg::Guid ImportTexture(const String resPath, const TextureImportDesc& config);
        AssetRefHandle<Texture> LoadTexture(const xg::Guid& guid);
        AssetRefHandle<Texture> LoadTexture(const String virtualPath);

        std::vector<xg::Guid> ImportStaticMesh(const String resPath, const StaticMeshImportDesc& desc);

        void Unload(const xg::Guid& guid);
        void Unload(const EAssetType assetType, const String virtualPath);

        void Delete(const xg::Guid& guid);
        void Delete(const EAssetType assetType, const String virtualPath);

        [[nodiscard]] AssetInfo GetAssetInfo(const xg::Guid guid) const;

        void SaveAllMetadataChanges();

    private:
        template <Asset AssetType>
        void operator()(const RefHandle<AssetType, AssetManager*>&, AssetType*)
        {
            IG_UNIMPLEMENTED();
        }

        template <Asset T>
        details::AssetCache<T>& GetAssetCache()
        {
            return static_cast<details::AssetCache<T>&>(GetTypelessCache(AssetTypeOf_v<T>));
        }

        details::TypelessAssetCache& GetTypelessCache(const EAssetType assetType);

        void InitAssetCaches();

        void DeleteInternal(const EAssetType assetType, const xg::Guid guid);

    private:
        Ptr<details::AssetMonitor> assetMonitor;
        std::vector<Ptr<details::TypelessAssetCache>> assetCaches;

        Ptr<TextureImporter> textureImporter;
    };

} // namespace ig