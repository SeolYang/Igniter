#include <Igniter.h>
#include <Core/Json.h>
#include <Core/Engine.h>
#include <Asset/AssetManager.h>
#include <Asset/Texture.h>
#include <Asset/Material.h>

namespace ig
{
    Json& MaterialLoadDesc::Serialize(Json& archive) const
    {
        IG_SERIALIZE_JSON_SIMPLE(MaterialLoadDesc, archive, DiffuseTexGuid);
        return archive;
    }

    const Json& MaterialLoadDesc::Deserialize(const Json& archive)
    {
        *this = {};
        IG_DESERIALIZE_JSON_SIMPLE(MaterialLoadDesc, archive, DiffuseTexGuid, Guid{DefaultTextureGuid});
        return archive;
    }

    Material::Material(AssetManager& assetManager, const Desc& snapshot, const ManagedAsset<Texture> diffuse)
        : assetManager(&assetManager), snapshot(snapshot), diffuse(diffuse)
    {
    }

    Material::~Material()
    {
        if (assetManager != nullptr)
        {
            assetManager->Unload(diffuse);
        }
    }
}    // namespace ig
