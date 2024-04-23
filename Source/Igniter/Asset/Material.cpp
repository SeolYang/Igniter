#include <Igniter.h>
#include <Core/Json.h>
#include <Asset/AssetCache.h>
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

    Material::Material(const Desc& snapshot, CachedAsset<Texture> diffuse) : snapshot(snapshot), diffuse(std::move(diffuse)) {}

    Material::~Material() {}
}    // namespace ig
