#include "Igniter/Igniter.h"
#include "Igniter/Core/Json.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Asset/AssetManager.h"
#include "Igniter/Asset/Texture.h"
#include "Igniter/Asset/Material.h"

namespace ig
{
    Json& MaterialAssetLoadDesc::Serialize(Json& archive) const
    {
        IG_SERIALIZE_TO_JSON(MaterialAssetLoadDesc, archive, DiffuseTexGuid);
        return archive;
    }

    const Json& MaterialAssetLoadDesc::Deserialize(const Json& archive)
    {
        *this = {};
        IG_DESERIALIZE_FROM_JSON_FALLBACK(MaterialAssetLoadDesc, archive, DiffuseTexGuid, Guid{DefaultTextureGuid});
        return archive;
    }

    Material::Material(AssetManager& assetManager, const Desc& snapshot, const ManagedAsset<Texture> diffuse)
        : assetManager(&assetManager)
        , snapshot(snapshot)
        , diffuse(diffuse)
    {
    }

    Material::~Material()
    {
        Destroy();
    }

    void Material::Destroy()
    {
        if (assetManager != nullptr)
        {
            assetManager->Unload(diffuse);
        }

        diffuse = {};
    }

    Material& Material::operator=(Material&& rhs) noexcept
    {
        Destroy();

        assetManager = std::exchange(rhs.assetManager, nullptr);
        snapshot = rhs.snapshot;
        diffuse = std::exchange(rhs.diffuse, {});

        return *this;
    }
} // namespace ig
