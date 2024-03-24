#pragma once
#include <Core/Handle.h>
#include <Core/Result.h>
#include <Asset/Common.h>

namespace ig
{
    enum class EAssetImportResult
    {
        Success,
        ImporterFailure,
    };

    template <typename AssetType>
    using AssetRefHandle = RefHandle<AssetType, class AssetManager*>;

    class AssetMonitor;
    struct Texture;
    struct TextureImportConfig;
    class TextureImporter;
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

        void Update();

        Result<xg::Guid, EAssetImportResult> ImportTexture(const String resPath, const TextureImportConfig& config);
        AssetRefHandle<Texture> LoadTexture(const xg::Guid& guid);
        AssetRefHandle<Texture> LoadTexture(const String virtualPath);

        void Unload(const xg::Guid& guid);
        void Unload(const EAssetType assetType, const String virtualPath);

    private:
        template <typename AssetType>
        void operator()(const RefHandle<AssetType, AssetManager*>&, AssetType*)
        {
            IG_UNIMPLEMENTED();
        }

    private:
        std::unique_ptr<AssetMonitor> assetMonitor;

        robin_hood::unordered_map<xg::Guid, uint32_t> guidReferenceCountTable;

        std::unique_ptr<TextureImporter> textureImporter;
        robin_hood::unordered_map<xg::Guid, Handle<Texture>> cachedTextures;
    };

} // namespace ig