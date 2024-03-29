#include <Igniter.h>
#include <Core/Log.h>
#include <Core/Serializable.h>
#include <D3D12/GpuBuffer.h>
#include <Render/GpuViewManager.h>
#include <Asset/AssetManager.h>
#include <Asset/AssetMonitor.h>
#include <Asset/AssetCache.h>
#include <Asset/Texture.h>
#include <Asset/Model.h>
#include <Asset/Utils.h>

IG_DEFINE_LOG_CATEGORY(AssetManager);

namespace ig
{
    AssetManager::AssetManager() : assetMonitor(std::make_unique<details::AssetMonitor>()),
                                   textureImporter(std::make_unique<TextureImporter>())
    {
        InitAssetCaches();
    }

    AssetManager::~AssetManager()
    {
        /* #sy_improvement 중간에 Pending 버퍼를 둬서, 실제로 저장하기 전까진 반영 X */
        /* 만약 반영전에 종료되거나, 취소한다면 Pending 버퍼 내용을 지울 것. */
        SaveAllMetadataChanges();
    }

    void AssetManager::InitAssetCaches()
    {
        assetCaches.emplace_back(std::make_unique<details::AssetCache<Texture>>());
        assetCaches.emplace_back(std::make_unique<details::AssetCache<StaticMesh>>());
    }

    details::TypelessAssetCache& AssetManager::GetTypelessCache(const EAssetType assetType)
    {
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

    xg::Guid AssetManager::ImportTexture(const String resPath, const TextureImportConfig& config)
    {
        Result<Texture::MetadataType, ETextureImportStatus> result = textureImporter->Import(resPath, config);
        if (!result.HasOwnership())
        {
            IG_LOG(AssetManager, Error, "Failed({}) to import texture \"{}\".", magic_enum::enum_name(result.GetStatus()), resPath.ToStringView());
            return {};
        }

        const Texture::MetadataType metadata = result.Take();
        const AssetInfo& assetInfo = metadata.first;

        if (assetMonitor->Contains(assetInfo.Type, assetInfo.VirtualPath))
        {
            Delete(EAssetType::Texture, assetInfo.VirtualPath);
        }
        assetMonitor->Create<Texture>(assetInfo, metadata.second);

        IG_LOG(AssetManager, Info, "\"{}\" imported as texture asset {}.", resPath.ToStringView(), assetInfo.Guid.str());
        return assetInfo.Guid;
    }

    void AssetManager::Unload(const EAssetType assetType, const String virtualPath)
    {
        if (assetType == EAssetType::Unknown)
        {
            IG_LOG(AssetManager, Error,
                   "Failed to unload {}. The asset type is unknown.",
                   virtualPath.ToStringView());
            return;
        }

        if (!assetMonitor->Contains(assetType, virtualPath))
        {
            IG_LOG(AssetManager, Error,
                   "Failed to unload {}. The virtual path is invisible to asset manager or invalid.",
                   virtualPath.ToStringView());
            return;
        }

        Unload(assetMonitor->GetGuid(assetType, virtualPath));
    }

    void AssetManager::Unload(const xg::Guid&)
    {
        IG_UNIMPLEMENTED();
    }

    void AssetManager::Delete(const xg::Guid& guid)
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

        if (assetMonitor->Contains(assetType, virtualPath))
        {
            IG_LOG(AssetManager, Error, "Failed to delete asset. The virtual path is invisible to asset manager or invalid.");
            return;
        }

        DeleteInternal(assetType, assetMonitor->GetGuid(assetType, virtualPath));
    }

    void AssetManager::DeleteInternal(const EAssetType assetType, const xg::Guid guid)
    {
        IG_CHECK(assetType != EAssetType::Unknown);
        IG_CHECK(guid.isValid() && assetMonitor->Contains(guid) && assetMonitor->GetAssetInfo(guid).Type == assetType);

        details::TypelessAssetCache& assetCache = GetTypelessCache(assetType);
        if (assetCache.IsCached(guid))
        {
            assetCache.Uncache(guid);
        }
        assetMonitor->Remove(guid);

        IG_LOG(AssetManager, Info, "Asset \"{}\" deleted.", guid.str());
    }

    void AssetManager::SaveAllMetadataChanges()
    {
        assetMonitor->SaveAllChanges();
    }
} // namespace ig