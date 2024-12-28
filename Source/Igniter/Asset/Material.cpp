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
        IG_SERIALIZE_JSON_SIMPLE(MaterialAssetLoadDesc, archive, DiffuseTexGuid);
        return archive;
    }

    const Json& MaterialAssetLoadDesc::Deserialize(const Json& archive)
    {
        *this = { };
        IG_DESERIALIZE_JSON_SIMPLE(MaterialAssetLoadDesc, archive, DiffuseTexGuid, Guid{ DefaultTextureGuid });
        return archive;
    }

    Material::Material(AssetManager& assetManager, const Desc& snapshot, const ManagedAsset<Texture> diffuse)
        : assetManager(&assetManager), snapshot(snapshot), diffuse(diffuse) { }

    Material::~Material()
    {
        if (assetManager != nullptr)
        {
            assetManager->Unload(diffuse);
        }
    }
} // namespace ig
