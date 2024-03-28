#include <Igniter.h>
#include <Core/Log.h>
#include <Core/Serializable.h>
#include <D3D12/GpuBuffer.h>
#include <Render/GpuViewManager.h>
#include <Asset/AssetManager.h>
#include <Asset/AssetMonitor.h>
#include <Asset/Texture.h>
#include <Asset/Model.h>
#include <Asset/Utils.h>

IG_DEFINE_LOG_CATEGORY(AssetManager);

namespace ig
{
    AssetManager::AssetManager() : assetMonitor(std::make_unique<AssetMonitor>()),
                                   textureImporter(std::make_unique<TextureImporter>())
    {
        InitAssetCaches();
    }

    AssetManager::~AssetManager()
    {
    }

    Result<xg::Guid, EAssetImportResult> AssetManager::ImportTexture(const String resPath, const TextureImportConfig& config)
    {
        IG_CHECK(textureImporter != nullptr);
        std::optional<AssetInfo> importedAssetInfo = textureImporter->Import(resPath, config);
        if (!importedAssetInfo)
        {
            return MakeFail<xg::Guid, EAssetImportResult::ImporterFailure>();
        }

        IG_CHECK(importedAssetInfo->IsValid());
        IG_CHECK(importedAssetInfo->VirtualPath.IsValid());
        if (assetMonitor->Contains(importedAssetInfo->Type, importedAssetInfo->VirtualPath))
        {
            Delete(importedAssetInfo->Type, importedAssetInfo->VirtualPath);
        }

        assetMonitor->Add(*importedAssetInfo);
        return MakeSuccess<xg::Guid, EAssetImportResult>(importedAssetInfo->Guid);
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

    void AssetManager::SaveAllChanges()
    {
        assetMonitor->SaveAllChanges();
        /* #sy_wip 에셋 메타데이터 저장 */
        // 에셋 순회 돌아서, 에셋 내부 메타데이터도 전부 업데이트 하는 방식 필요
        // 에셋의 타입에 내부적으로 무조건 'Metadata' 를 들고있게 하는게 좋아보임.
        // 1. Metadata type의 정의
        // 2. Metadata 멤버 정의
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

    void AssetManager::InitAssetCaches()
    {
        assetCaches.emplace_back(std::make_unique<AssetCache<Texture>>());
        assetCaches.emplace_back(std::make_unique<AssetCache<StaticMesh>>());
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

} // namespace ig