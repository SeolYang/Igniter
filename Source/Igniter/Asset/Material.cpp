#include <Igniter.h>
#include <Core/Json.h>
#include <Asset/AssetCache.h>
#include <Asset/Texture.h>
#include <Asset/Material.h>

namespace ig
{
    json& MaterialLoadDesc::Serialize(json& archive) const
    {
        IG_SERIALIZE_JSON_SIMPLE(MaterialLoadDesc, archive, DiffuseTexGuid);
        return archive;
    }

    const json& MaterialLoadDesc::Deserialize(const json& archive)
    {
        *this = {};
        IG_DESERIALIZE_JSON_SIMPLE(MaterialLoadDesc, archive, DiffuseTexGuid, Guid{ DefaultTextureGuid });
        return archive;
    }

    Material::Material(Desc snapshot, CachedAsset<Texture> diffuse)
        : snapshot(snapshot),
          diffuse(std::move(diffuse))
    {
    }

    Material::~Material()
    {
    }

    Texture& Material::GetDiffuse()
    {
        /* #sy_tood Diffuse가 invalidated 된 경우, Engine Default 를 대신 설정 */
        IG_CHECK(diffuse);
        return *diffuse;
    }
} // namespace ig