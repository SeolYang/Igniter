#pragma once
#include <Asset/Common.h>
#include <Asset/Texture.h>
#include <Filesystem/AsyncFileTracker.h>

namespace ig
{
    class AssetManager final
    {
        friend class RefHandle<Texture, AssetManager*>;
        friend class Handle<Texture, AssetManager*>;

    public:
        AssetManager();
        AssetManager(const AssetManager&) = delete;
        AssetManager(AssetManager&&) noexcept = delete;
        ~AssetManager();

        AssetManager& operator=(const AssetManager&) = delete;
        AssetManager& operator=(AssetManager&&) noexcept = delete;

        bool ImportTexture(const String resPath, std::optional<TextureImportConfig> config = std::nullopt, const bool bIsPersistent = false);
        RefHandle<Texture, AssetManager*> LoadTexture(const xg::Guid& guid);

        void Unload(const xg::Guid& guid);

    private:
        template <typename AssetType>
        void operator()(const RefHandle<AssetType, AssetManager*>&, AssetType*)
        {
            IG_UNIMPLEMENTED();
        }

        /* "/Assets" 폴더에서 리소스의 메타데이터를 읽어와서, 초기 nameGuidTable과 guidAssetInfoTable 설정 */
        void ParseAssetDirectory();

    private:
        robin_hood::unordered_map<String, xg::Guid> nameGuidTable;
        robin_hood::unordered_map<xg::Guid, AssetInfo> guidAssetInfoTable;

    };

} // namespace ig