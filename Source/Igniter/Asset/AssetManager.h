#pragma once
#include <Asset/Common.h>
#include <Asset/Texture.h>

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

    private:
    };

} // namespace ig