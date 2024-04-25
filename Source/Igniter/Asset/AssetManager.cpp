#include <Igniter.h>
#include <D3D12/GpuTexture.h>
#include <D3D12/GpuBuffer.h>
#include <Asset/TextureImporter.h>
#include <Asset/StaticMeshImporter.h>
#include <Asset/MaterialImporter.h>
#include <Asset/AssetManager.h>

namespace ig
{
    AssetManager::AssetManager(RenderContext& renderContext)
        : assetMonitor(MakePtr<details::AssetMonitor>())
        , textureImporter(MakePtr<TextureImporter>())
        , textureLoader(MakePtr<TextureLoader>(renderContext))
        , staticMeshImporter(MakePtr<StaticMeshImporter>(*this))
        , staticMeshLoader(MakePtr<StaticMeshLoader>(renderContext, *this))
        , materialImporter(MakePtr<MaterialImporter>(*this))
        , materialLoader(MakePtr<MaterialLoader>(*this))
    {
        assetCaches.emplace_back(MakePtr<details::AssetCache<Texture>>());
        assetCaches.emplace_back(MakePtr<details::AssetCache<StaticMesh>>());
        assetCaches.emplace_back(MakePtr<details::AssetCache<Material>>());
    }

    AssetManager::~AssetManager()
    {
        /* #sy_todo_priority 아직 캐싱되어 있는 에셋에 대한 정보 출력 */
        SaveAllChanges();
    }

    details::TypelessAssetCache& AssetManager::GetTypelessCache(const EAssetCategory assetType)
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

    const details::TypelessAssetCache& AssetManager::GetTypelessCache(const EAssetCategory assetType) const
    {
        IG_CHECK(assetCaches.size() > 0);

        details::TypelessAssetCache* cachePtr = nullptr;
        for (const Ptr<details::TypelessAssetCache>& cache : assetCaches)
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

    void AssetManager::RegisterEngineDefault()
    {
        /* #sy_wip 기본 에셋 Virtual Path들도 AssetManager 컴파일 타임 상수로 통합 */
        AssetInfo defaultTexInfo{Guid{DefaultTextureGuid}, Texture::EngineDefault, EAssetCategory::Texture, EAssetScope::Engine};
        RegisterEngineInternalAsset<Texture>(defaultTexInfo.GetVirtualPath(), textureLoader->MakeDefault(defaultTexInfo));

        AssetInfo defaultWhiteTexInfo{Guid{DefaultWhiteTextureGuid}, Texture::EngineDefaultWhite, EAssetCategory::Texture, EAssetScope::Engine};
        RegisterEngineInternalAsset<Texture>(
            defaultWhiteTexInfo.GetVirtualPath(), textureLoader->MakeMonochrome(defaultWhiteTexInfo, Color{1.f, 1.f, 1.f}));

        AssetInfo defaultBlackTexInfo{Guid{DefaultBlackTextureGuid}, Texture::EngineDefaultBlack, EAssetCategory::Texture, EAssetScope::Engine};
        RegisterEngineInternalAsset<Texture>(
            defaultBlackTexInfo.GetVirtualPath(), textureLoader->MakeMonochrome(defaultBlackTexInfo, Color{0.f, 0.f, 0.f}));

        AssetInfo defaultMatInfo{Guid{DefaultMaterialGuid}, Material::EngineDefault, EAssetCategory::Material, EAssetScope::Engine};
        RegisterEngineInternalAsset<Material>(defaultMatInfo.GetVirtualPath(), materialLoader->MakeDefault(defaultMatInfo));
    }

    void AssetManager::UnRegisterEngineDefault()
    {
        Delete(Guid{DefaultTextureGuid});
        Delete(Guid{DefaultWhiteTextureGuid});
        Delete(Guid{DefaultBlackTextureGuid});
        Delete(Guid{DefaultMaterialGuid});
    }

    Guid AssetManager::Import(const String resPath, const TextureImportDesc& config)
    {
        Result<Texture::Desc, ETextureImportStatus> result = textureImporter->Import(resPath, config);
        const std::optional<Guid> guidOpt{ImportImpl<Texture>(resPath, result)};
        if (!guidOpt)
        {
            return Guid{};
        }

        bIsDirty = true;
        return *guidOpt;
    }

    Handle<Texture> AssetManager::LoadTexture(const Guid& guid)
    {
        Handle<Texture> cachedTex{LoadImpl<Texture>(guid, *textureLoader)};
        if (!cachedTex)
        {
            return LoadImpl<Texture>(Guid{DefaultTextureGuid}, *textureLoader);
        }

        return cachedTex;
    }

    Handle<Texture> AssetManager::LoadTexture(const String virtualPath)
    {
        if (!IsValidVirtualPath(virtualPath))
        {
            IG_LOG(AssetManager, Error, "Load Texture: Invalid Virtual Path {}", virtualPath);
            return LoadImpl<Texture>(Guid{DefaultTextureGuid}, *textureLoader);
        }

        if (!assetMonitor->Contains(EAssetCategory::Texture, virtualPath))
        {
            IG_LOG(AssetManager, Error, "Texture \"{}\" is invisible to asset manager.", virtualPath);
            return LoadImpl<Texture>(Guid{DefaultTextureGuid}, *textureLoader);
        }

        return LoadTexture(assetMonitor->GetGuid(EAssetCategory::Texture, virtualPath));
    }

    std::vector<Guid> AssetManager::Import(const String resPath, const StaticMeshImportDesc& desc)
    {
        std::vector<Result<StaticMesh::Desc, EStaticMeshImportStatus>> results = staticMeshImporter->Import(resPath, desc);
        std::vector<Guid> output;
        output.reserve(results.size());
        for (Result<StaticMesh::Desc, EStaticMeshImportStatus>& result : results)
        {
            std::optional<Guid> guidOpt{ImportImpl<StaticMesh>(resPath, result)};
            if (guidOpt)
            {
                output.emplace_back(*guidOpt);
            }
        }

        bIsDirty = true;
        return output;
    }

    Handle<StaticMesh> AssetManager::LoadStaticMesh(const Guid& guid)
    {
        return LoadImpl<StaticMesh>(guid, *staticMeshLoader);
    }

    Handle<StaticMesh> AssetManager::LoadStaticMesh(const String virtualPath)
    {
        if (!IsValidVirtualPath(virtualPath))
        {
            IG_LOG(AssetManager, Error, "Load Static Mesh: Invalid Virtual Path {}", virtualPath);
            return Handle<StaticMesh>{};
        }

        if (!assetMonitor->Contains(EAssetCategory::StaticMesh, virtualPath))
        {
            IG_LOG(AssetManager, Error, "Static mesh \"{}\" is invisible to asset manager.", virtualPath);
            return Handle<StaticMesh>{};
        }

        return LoadImpl<StaticMesh>(assetMonitor->GetGuid(EAssetCategory::StaticMesh, virtualPath), *staticMeshLoader);
    }

    Guid AssetManager::Import(const String virtualPath, const MaterialCreateDesc& createDesc)
    {
        if (!IsValidVirtualPath(virtualPath))
        {
            IG_LOG(AssetManager, Error, "Import Material: Invalid Virtual Path {}", virtualPath);
            return Guid{};
        }

        Result<Material::Desc, EMaterialCreateStatus> result{materialImporter->Import(AssetInfo{virtualPath, EAssetCategory::Material}, createDesc)};
        std::optional<Guid> guidOpt{ImportImpl<Material>(virtualPath, result)};
        if (!guidOpt)
        {
            return Guid{};
        }

        bIsDirty = true;
        return *guidOpt;
    }

    Handle<Material> AssetManager::LoadMaterial(const Guid& guid)
    {
        Handle<Material> cachedMat{LoadImpl<Material>(guid, *materialLoader)};
        if (!cachedMat)
        {
            return LoadImpl<Material>(Guid{DefaultMaterialGuid}, *materialLoader);
        }

        return cachedMat;
    }

    Handle<Material> AssetManager::LoadMaterial(const String virtualPath)
    {
        if (!IsValidVirtualPath(virtualPath))
        {
            IG_LOG(AssetManager, Error, "Load Material: Invalid Virtual Path {}", virtualPath);
            return LoadImpl<Material>(Guid{DefaultMaterialGuid}, *materialLoader);
        }

        if (!assetMonitor->Contains(EAssetCategory::Material, virtualPath))
        {
            IG_LOG(AssetManager, Error, "Material \"{}\" is invisible to asset manager.", virtualPath);
            return LoadImpl<Material>(Guid{DefaultMaterialGuid}, *materialLoader);
        }

        return LoadMaterial(assetMonitor->GetGuid(EAssetCategory::Material, virtualPath));
    }

    void AssetManager::Delete(const Guid& guid)
    {
        if (!assetMonitor->Contains(guid))
        {
            IG_LOG(AssetManager, Error, "Failed to delete asset. The asset guid (\"{}\") is invisible to asset manager or invalid.", guid.str());
            return;
        }

        DeleteImpl(assetMonitor->GetAssetInfo(guid).GetCategory(), guid);
    }

    void AssetManager::Delete(const EAssetCategory assetType, const String virtualPath)
    {
        if (assetType == EAssetCategory::Unknown)
        {
            IG_LOG(AssetManager, Error, "Failed to delete asset. The asset type is unknown.");
            return;
        }

        if (!assetMonitor->Contains(assetType, virtualPath))
        {
            IG_LOG(AssetManager, Error, "Failed to delete asset. The virtual path is invisible to asset manager or invalid.");
            return;
        }

        DeleteImpl(assetType, assetMonitor->GetGuid(assetType, virtualPath));
    }

    void AssetManager::DeleteImpl(const EAssetCategory assetType, const Guid& guid)
    {
        UniqueLock assetLock{GetAssetMutex(guid)};
        IG_CHECK(assetType != EAssetCategory::Unknown);
        IG_CHECK(guid.isValid() && assetMonitor->Contains(guid) && assetMonitor->GetAssetInfo(guid).GetCategory() == assetType);

        details::TypelessAssetCache& assetCache = GetTypelessCache(assetType);
        if (assetCache.IsCached(guid))
        {
            assetCache.Invalidate(guid);
        }
        assetMonitor->Remove(guid);

        IG_LOG(AssetManager, Info, "Asset \"{}\" deleted.", guid);
        bIsDirty = true;
    }

    AssetManager::AssetMutex& AssetManager::GetAssetMutex(const Guid& guid)
    {
        UniqueLock lock{assetMutexTableMutex};
        if (!assetMutexTable.contains(guid))
        {
            assetMutexTable[guid];
        }

        return assetMutexTable[guid];
    }

    void AssetManager::SaveAllChanges()
    {
        assetMonitor->SaveAllChanges();
    }

    std::optional<AssetInfo> AssetManager::GetAssetInfo(const Guid& guid) const
    {
        if (!assetMonitor->Contains(guid))
        {
            IG_LOG(AssetManager, Error, "\"{}\" is invisible to asset manager.", guid);
            return std::nullopt;
        }

        return assetMonitor->GetAssetInfo(guid);
    }

    std::vector<AssetManager::Snapshot> AssetManager::TakeSnapshots() const
    {
        UnorderedMap<Guid, Snapshot> intermediateSnapshots{};
        std::vector<AssetInfo> assetInfoSnapshots{assetMonitor->TakeSnapshots()};
        for (const AssetInfo& assetInfoSnapshot : assetInfoSnapshots)
        {
            if (assetInfoSnapshot.IsValid())
            {
                intermediateSnapshots[assetInfoSnapshot.GetGuid()] = Snapshot{.Info = assetInfoSnapshot, .RefCount = 0};
            }
        }

        for (const Ptr<details::TypelessAssetCache>& assetCache : assetCaches)
        {
            std::vector<details::TypelessAssetCache::Snapshot> assetCacheSnapshots{assetCache->TakeSnapshots()};
            for (const details::TypelessAssetCache::Snapshot& cacheSnapshot : assetCacheSnapshots)
            {
                IG_CHECK(intermediateSnapshots.contains(cacheSnapshot.Guid));
                Snapshot& snapshot{intermediateSnapshots.at(cacheSnapshot.Guid)};
                snapshot.RefCount = cacheSnapshot.RefCount;
            }
        }

        std::vector<Snapshot> snapshots{};
        snapshots.reserve(intermediateSnapshots.size());
        for (const auto& guidSnapshot : intermediateSnapshots)
        {
            snapshots.emplace_back(guidSnapshot.second);
        }
        return snapshots;
    }

    void AssetManager::Update()
    {
        if (bIsDirty)
        {
            ZoneScoped;
            assetModifiedEvent.Notify(*this);
            bIsDirty = false;
        }
    }
}    // namespace ig
