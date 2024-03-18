#pragma once
#include <Asset/Common.h>
#include <Asset/Texture.h>

namespace ig
{
    class AssetCache final
    {
        friend class RefHandle<Texture, AssetCache*>;
        friend class Handle<Texture, AssetCache*>;

    public:
        AssetCache();
        AssetCache(const AssetCache&) = delete;
        AssetCache(AssetCache&&) noexcept = delete;
        ~AssetCache();

        AssetCache& operator=(const AssetCache&) = delete;
        AssetCache& operator=(AssetCache&&) noexcept = delete;

        bool ImportTexture(const String resPath, std::optional<TextureImportConfig> config = std::nullopt, const bool bIsPersistent = false);
        RefHandle<Texture, AssetCache*> LoadTexture(const xg::Guid& guid);

        void Unload(const xg::Guid& guid);

    private:
        template <typename AssetType>
        void operator()(const RefHandle<AssetType, AssetCache*>&, AssetType*)
        {
            IG_UNIMPLEMENTED();
        }

    private:
    };

} // namespace ig