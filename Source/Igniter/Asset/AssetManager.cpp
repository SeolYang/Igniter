#include <Igniter.h>
#include <Core/Serializable.h>
#include <Core/TypeUtils.h>
#include <D3D12/GpuTexture.h>
#include <D3D12/GpuBuffer.h>
#include <Render/GpuViewManager.h>
#include <Asset/Utils.h>
#include <Asset/TextureImporter.h>
#include <Asset/TextureLoader.h>
#include <Asset/StaticMeshImporter.h>
#include <Asset/StaticMeshLoader.h>
#include <Asset/AssetManager.h>

namespace ig
{
    AssetManager::AssetManager(HandleManager& handleManager, RenderDevice& renderDevice, GpuUploader& gpuUploader, GpuViewManager& gpuViewManager)
        : assetMonitor(std::make_unique<details::AssetMonitor>()),
          textureImporter(std::make_unique<TextureImporter>()),
          textureLoader(std::make_unique<TextureLoader>(handleManager, renderDevice, gpuUploader, gpuViewManager)),
          staticMeshImporter(std::make_unique<StaticMeshImporter>(*this)),
          staticMeshLoader(std::make_unique<StaticMeshLoader>(handleManager, renderDevice, gpuUploader, gpuViewManager))
    {
        InitAssetCaches(handleManager);
    }

    AssetManager::~AssetManager()
    {
        /* #sy_improvement 중간에 Pending 버퍼를 둬서, 실제로 저장하기 전까진 반영 X */
        /* 만약 반영전에 종료되거나, 취소한다면 Pending 버퍼 내용을 지울 것. */
        SaveAllChanges();
    }

    void AssetManager::InitAssetCaches(HandleManager& handleManager)
    {
        assetCaches.emplace_back(std::make_unique<details::AssetCache<Texture>>(handleManager));
        assetCaches.emplace_back(std::make_unique<details::AssetCache<StaticMesh>>(handleManager));
    }

    details::TypelessAssetCache& AssetManager::GetTypelessCache(const EAssetType assetType)
    {
        IG_CHECK(assetCaches.size() > 0);

        details::TypelessAssetCache* cachePtr = nullptr;
        for (Ptr<details::TypelessAssetCache>& cache : assetCaches)
        {
            if (cache->GetAssetType() == assetType)
            {
                cachePtr = cache.get();
                break;
            }
        }

        IG_CHECK(cachePtr != nullptr);
        return *cachePtr;
    }

    xg::Guid AssetManager::ImportTexture(const String resPath, const TextureImportDesc& config)
    {
        Result<Texture::Desc, ETextureImportStatus> result = textureImporter->Import(resPath, config);
        if (!result.HasOwnership())
        {
            IG_LOG(AssetManager, Error, "Failed({}) to import texture \"{}\".",
                   magic_enum::enum_name(result.GetStatus()), resPath.ToStringView());
            return {};
        }

        const Texture::Desc metadata = result.Take();
        const AssetInfo& assetInfo = metadata.Info;

        if (assetMonitor->Contains(assetInfo.Type, assetInfo.VirtualPath))
        {
            Delete(assetInfo.Type, assetInfo.VirtualPath);
        }
        assetMonitor->Create<Texture>(assetInfo, metadata.LoadDescriptor);

        IG_LOG(AssetManager, Info, "\"{}\" imported as texture asset {}({}).", resPath.ToStringView(), assetInfo.VirtualPath.ToStringView(), assetInfo.Guid.str());
        return assetInfo.Guid;
    }

    CachedAsset<Texture> AssetManager::LoadTexture(const xg::Guid guid)
    {
        return LoadInternal<Texture>(guid, *textureLoader);
    }

    CachedAsset<Texture> AssetManager::LoadTexture(const String virtualPath)
    {
        if (!assetMonitor->Contains(EAssetType::Texture, virtualPath))
        {
            IG_LOG(AssetManager, Error, "Texture asset \"{}\" is invisible to asset manager.", virtualPath);
            return CachedAsset<Texture>{};
        }

        return LoadInternal<Texture>(assetMonitor->GetGuid(EAssetType::Texture, virtualPath), *textureLoader);
    }

    std::vector<xg::Guid> AssetManager::ImportStaticMesh(const String resPath, const StaticMeshImportDesc& desc)
    {
        std::vector<Result<StaticMesh::Desc, EStaticMeshImportStatus>> results = staticMeshImporter->ImportStaticMesh(resPath, desc);
        std::vector<xg::Guid> output;
        output.reserve(results.size());
        for (Result<StaticMesh::Desc, EStaticMeshImportStatus>& result : results)
        {
            if (!result.HasOwnership())
            {
                IG_LOG(AssetManager, Error, "Failed({}) to import static mesh \"{}\".",
                       magic_enum::enum_name(result.GetStatus()), resPath.ToStringView());
                continue;
            }

            const StaticMesh::Desc metadata = result.Take();
            const AssetInfo& assetInfo = metadata.Info;

            if (assetMonitor->Contains(assetInfo.Type, assetInfo.VirtualPath))
            {
                Delete(assetInfo.Type, assetInfo.VirtualPath);
            }
            assetMonitor->Create<StaticMesh>(assetInfo, metadata.LoadDescriptor);

            IG_LOG(AssetManager, Info, "\"{}\" imported as static mesh asset {}({})", resPath.ToStringView(), assetInfo.VirtualPath.ToStringView(), assetInfo.Guid.str());
            output.emplace_back(assetInfo.Guid);
        }

        return output;
    }

    CachedAsset<StaticMesh> AssetManager::LoadStaticMesh(const xg::Guid guid)
    {
        return LoadInternal<StaticMesh>(guid, *staticMeshLoader);
    }

    CachedAsset<StaticMesh> AssetManager::LoadStaticMesh(const String virtualPath)
    {
        if (!assetMonitor->Contains(EAssetType::StaticMesh, virtualPath))
        {
            IG_LOG(AssetManager, Error, "Static mesh asset \"{}\" is invisible to asset manager.", virtualPath);
            return CachedAsset<StaticMesh>{};
        }

        return LoadInternal<StaticMesh>(assetMonitor->GetGuid(EAssetType::StaticMesh, virtualPath), *staticMeshLoader);
    }

    void AssetManager::Delete(const xg::Guid guid)
    {
        if (!assetMonitor->Contains(guid))
        {
            IG_LOG(AssetManager, Error,
                   "Failed to delete asset. The asset guid (\"{}\") is invisible to asset manager or invalid.",
                   guid.str());
            return;
        }

        DeleteInternal(assetMonitor->GetAssetInfo(guid).Type, guid);
    }

    void AssetManager::Delete(const EAssetType assetType, const String virtualPath)
    {
        if (assetType == EAssetType::Unknown)
        {
            IG_LOG(AssetManager, Error, "Failed to delete asset. The asset type is unknown.");
            return;
        }

        if (!assetMonitor->Contains(assetType, virtualPath))
        {
            IG_LOG(AssetManager, Error, "Failed to delete asset. The virtual path is invisible to asset manager or invalid.");
            return;
        }

        DeleteInternal(assetType, assetMonitor->GetGuid(assetType, virtualPath));
    }

    AssetInfo AssetManager::GetAssetInfo(const xg::Guid guid) const
    {
        if (!assetMonitor->Contains(guid))
        {
            IG_LOG(AssetManager, Error, "\"{}\" is invisible to asset manager.", guid.str());
        }

        return assetMonitor->GetAssetInfo(guid);
    }

    void AssetManager::DeleteInternal(const EAssetType assetType, const xg::Guid guid)
    {
        IG_CHECK(assetType != EAssetType::Unknown);
        IG_CHECK(guid.isValid() && assetMonitor->Contains(guid) && assetMonitor->GetAssetInfo(guid).Type == assetType);

        details::TypelessAssetCache& assetCache = GetTypelessCache(assetType);
        if (assetCache.IsCached(guid))
        {
            assetCache.Invalidate(guid);
        }
        assetMonitor->Remove(guid);

        IG_LOG(AssetManager, Info, "Asset \"{}\" deleted.", guid.str());
    }

    void AssetManager::SaveAllChanges()
    {
        assetMonitor->SaveAllChanges();
    }
} // namespace ig