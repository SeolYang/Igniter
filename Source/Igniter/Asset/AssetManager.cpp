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

    Result<xg::Guid, EAssetImportResult> AssetManager::ImportTexture(const String resPath, const TextureImportConfig& config)
    {
        IG_CHECK(textureImporter != nullptr);
        std::optional<std::pair<AssetInfo, Texture::MetadataType>> assetMetadata = textureImporter->Import(resPath, config);
        if (!assetMetadata)
        {
            return MakeFail<xg::Guid, EAssetImportResult::ImporterFailure>();
        }

        const AssetInfo& assetInfo = assetMetadata->first;

        IG_CHECK(assetInfo.IsValid());
        IG_CHECK(assetInfo.VirtualPath.IsValid());
        if (assetMonitor->Contains(assetInfo.Type, assetInfo.VirtualPath))
        {
            Delete(assetInfo.Type, assetInfo.VirtualPath);
        }

        assetMonitor->Create<Texture>(assetInfo, assetMetadata->second);
        return MakeSuccess<xg::Guid, EAssetImportResult>(assetInfo.Guid);
    }

    void AssetManager::Unload(const EAssetType assetType, const String virtualPath)
    {
        Unload(assetMonitor->GetGuid(assetType, virtualPath));
    }

    void AssetManager::Unload(const xg::Guid&)
    {
        IG_UNIMPLEMENTED();
    }

    void AssetManager::Delete(const xg::Guid& guid)
    {
        IG_CHECK(guid.isValid());
        IG_CHECK(assetMonitor->Contains(guid));

        std::optional<AssetInfo> assetInfo = assetMonitor->GetAssetInfo(guid);
        IG_CHECK(assetInfo);
        DeleteInternal(assetInfo->Type, guid);
    }

    void AssetManager::Delete(const EAssetType assetType, const String virtualPath)
    {
        DeleteInternal(assetType, assetMonitor->GetGuid(assetType, virtualPath));
    }

    void AssetManager::DeleteInternal(const EAssetType assetType, const xg::Guid guid)
    {
        IG_CHECK(assetType != EAssetType::Unknown);
        IG_CHECK(guid.isValid());

        details::TypelessAssetCache& assetCache = GetTypelessCache(assetType);
        if (assetCache.IsCached(guid))
        {
            assetCache.Uncache(guid);
        }

        assetMonitor->Remove(guid);
    }

    void AssetManager::SaveAllMetadataChanges()
    {
        assetMonitor->SaveAllChanges();
    }
} // namespace ig