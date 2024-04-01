#include <Igniter.h>
#include <Core/Timer.h>
#include <Core/JsonUtils.h>
#include <Core/Serializable.h>
#include <Filesystem/Utils.h>
#include <Asset/AssetCache.h>
#include <Asset/Texture.h>
#include <Asset/Material.h>

namespace ig
{
    json& MaterialLoadDesc::Serialize(json& archive) const
    {
        const auto& desc{ *this };
        IG_SERIALIZE_JSON(MaterialLoadDesc, archive, desc, DiffuseVirtualPath);
        return archive;
    }

    const json& MaterialLoadDesc::Deserialize(const json& archive)
    {
        auto& desc{ *this };
        desc = {};
        IG_DESERIALIZE_JSON(MaterialLoadDesc, archive, desc, DiffuseVirtualPath, String{ /* #sy_todo 추후에 엔진 기본 텍스처로 대체 */ });
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
        IG_CHECK(diffuse);
        return *diffuse;
    }

    Result<Material::Desc, EMaterialCreateStatus> Material::Create(const String virtualPath, MaterialCreateDesc desc)
    {
        const AssetInfo assetInfo{
            .CreationTime = Timer::Now(),
            .Guid = xg::newGuid(),
            .VirtualPath = virtualPath,
            .Type = EAssetType::Material,
        };

        String diffuseVirtualPath{};
        if (desc.Diffuse)
        {
            const Texture::Desc& snapshot{ desc.Diffuse->GetSnapshot() };
            diffuseVirtualPath = snapshot.Info.VirtualPath;
        }
        else
        {
            /* #sy_todo
             * 엔진 기본 텍스처 에셋(in-lined)
             * 주어진 desc 내부의 텍스처가 invalid 한 경우, 인라인 텍스처 에셋으로 대체
             */
        }

        const Material::LoadDesc loadDesc{
            .DiffuseVirtualPath = diffuseVirtualPath
        };

        json serializedMeta{};
        serializedMeta << assetInfo << loadDesc;

        const fs::path metadataPath{ MakeAssetMetadataPath(EAssetType::Material, assetInfo.Guid) };
        IG_CHECK(!metadataPath.empty());
        if (!SaveJsonToFile(metadataPath, serializedMeta))
        {
            return MakeFail<Material::Desc, EMaterialCreateStatus::FailedSaveMetadata>();
        }

        const fs::path assetPath{ MakeAssetPath(EAssetType::Material, assetInfo.Guid) };
        if (!fs::exists(assetPath) && !SaveBlobToFile(assetPath, std::array<uint8_t, 1>{ 0 }))
        {
            return MakeFail<Material::Desc, EMaterialCreateStatus::FailedSaveAsset>();
        }

        return MakeSuccess<Material::Desc, EMaterialCreateStatus>(Desc{
            .Info = assetInfo,
            .LoadDescriptor = loadDesc });
    }
} // namespace ig