#include <Igniter.h>
#include <Core/Timer.h>
#include <Asset/AssetCache.h>
#include <Asset/Texture.h>
#include <Asset/Material.h>

namespace ig
{
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

    Material::Desc Material::Create(const String virtualPath, MaterialCreateDesc desc)
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

        return Desc{
            .Info = assetInfo,
            .LoadDescriptor = loadDesc
        };
    }
} // namespace ig