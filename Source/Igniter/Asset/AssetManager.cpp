#include <Igniter.h>
#include <D3D12/GpuTexture.h>
#include <D3D12/GpuBuffer.h>
#include <Render/GpuViewManager.h>
#include <Asset/TextureImporter.h>
#include <Asset/TextureLoader.h>
#include <Asset/StaticMeshImporter.h>
#include <Asset/StaticMeshLoader.h>
#include <Asset/MaterialImporter.h>
#include <Asset/MaterialLoader.h>
#include <Asset/AssetManager.h>

namespace ig
{
    AssetManager::AssetManager(HandleManager& handleManager, RenderDevice& renderDevice, GpuUploader& gpuUploader, GpuViewManager& gpuViewManager)
        : assetMonitor(std::make_unique<details::AssetMonitor>()),
          textureImporter(std::make_unique<TextureImporter>()),
          textureLoader(std::make_unique<TextureLoader>(handleManager, renderDevice, gpuUploader, gpuViewManager)),
          staticMeshImporter(std::make_unique<StaticMeshImporter>(*this)),
          staticMeshLoader(std::make_unique<StaticMeshLoader>(handleManager, renderDevice, gpuUploader, gpuViewManager)),
          materialImporter(std::make_unique<MaterialImporter>(*this)),
          materialLoader(std::make_unique<MaterialLoader>(*this))
    {
        InitAssetCaches(handleManager);
        InitEngineDefaultAssets();
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
        assetCaches.emplace_back(std::make_unique<details::AssetCache<Material>>(handleManager));
    }

    void AssetManager::InitEngineDefaultAssets()
    {
        AssetInfo defaultTexInfo{ Texture::EngineDefault, EAssetType::Texture };
        defaultTexInfo.MarkAsEngineDefault();
        defaultTexInfo.SetGuid(Guid{ DefaultTextureGuid });
        RegisterEngineDefault<Texture>(defaultTexInfo.GetVirtualPath(), textureLoader->MakeDefault(defaultTexInfo));

        AssetInfo defaultWhiteTexInfo{ Texture::EngineDefaultWhite, EAssetType::Texture };
        defaultWhiteTexInfo.MarkAsEngineDefault();
        defaultWhiteTexInfo.SetGuid(Guid{ DefaultWhiteTextureGuid });
        RegisterEngineDefault<Texture>(defaultWhiteTexInfo.GetVirtualPath(),
                                       textureLoader->MakeMonochrome(defaultWhiteTexInfo, Color{ 1.f, 1.f, 1.f }));

        AssetInfo defaultBlackTexInfo{ Texture::EngineDefaultBlack, EAssetType::Texture };
        defaultBlackTexInfo.MarkAsEngineDefault();
        defaultBlackTexInfo.SetGuid(Guid{ DefaultBlackTextureGuid });
        RegisterEngineDefault<Texture>(defaultBlackTexInfo.GetVirtualPath(),
                                       textureLoader->MakeMonochrome(defaultBlackTexInfo, Color{ 0.f, 0.f, 0.f }));

        AssetInfo defaultMatInfo{ Material::EngineDefault, EAssetType::Material };
        defaultMatInfo.MarkAsEngineDefault();
        defaultMatInfo.SetGuid(Guid{ DefaultMaterialGuid });
        RegisterEngineDefault<Material>(defaultMatInfo.GetVirtualPath(), materialLoader->MakeDefault(defaultMatInfo));
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

    Guid AssetManager::ImportTexture(const String resPath, const TextureImportDesc& config)
    {
        Result<Texture::Desc, ETextureImportStatus> result = textureImporter->Import(resPath, config);
        std::optional<Guid> guidOpt{ ImportInternal<Texture>(resPath, result) };
        if (!guidOpt)
        {
            return Guid{};
        }

        assetModifiedEvent.Notify(*this);
        return *guidOpt;
    }

    CachedAsset<Texture> AssetManager::LoadTexture(const Guid guid)
    {
        CachedAsset<Texture> cachedTex{ LoadInternal<Texture>(guid, *textureLoader) };
        if (!cachedTex)
        {
            return LoadInternal<Texture>(Guid{ DefaultTextureGuid }, *textureLoader);
        }

        return cachedTex;
    }

    CachedAsset<Texture> AssetManager::LoadTexture(const String virtualPath)
    {
        if (!IsValidVirtualPath(virtualPath))
        {
            IG_LOG(AssetManager, Error, "Load Texture: Invalid Virtual Path {}", virtualPath);
            return LoadInternal<Texture>(Guid{ DefaultTextureGuid }, *textureLoader);
        }

        if (!assetMonitor->Contains(EAssetType::Texture, virtualPath))
        {
            IG_LOG(AssetManager, Error, "Texture \"{}\" is invisible to asset manager.", virtualPath);
            return LoadInternal<Texture>(Guid{ DefaultTextureGuid }, *textureLoader);
        }

        return LoadTexture(assetMonitor->GetGuid(EAssetType::Texture, virtualPath));
    }

    std::vector<Guid> AssetManager::ImportStaticMesh(const String resPath, const StaticMeshImportDesc& desc)
    {
        std::vector<Result<StaticMesh::Desc, EStaticMeshImportStatus>> results = staticMeshImporter->ImportStaticMesh(resPath, desc);
        std::vector<Guid> output;
        output.reserve(results.size());
        for (Result<StaticMesh::Desc, EStaticMeshImportStatus>& result : results)
        {
            std::optional<Guid> guidOpt{ ImportInternal<StaticMesh>(resPath, result) };
            if (guidOpt)
            {
                output.emplace_back(*guidOpt);
            }
        }

        assetModifiedEvent.Notify(*this);
        return output;
    }

    CachedAsset<StaticMesh> AssetManager::LoadStaticMesh(const Guid guid)
    {
        return LoadInternal<StaticMesh>(guid, *staticMeshLoader);
    }

    CachedAsset<StaticMesh> AssetManager::LoadStaticMesh(const String virtualPath)
    {
        if (!IsValidVirtualPath(virtualPath))
        {
            IG_LOG(AssetManager, Error, "Load Static Mesh: Invalid Virtual Path {}", virtualPath);
            return CachedAsset<StaticMesh>{};
        }

        if (!assetMonitor->Contains(EAssetType::StaticMesh, virtualPath))
        {
            IG_LOG(AssetManager, Error, "Static mesh \"{}\" is invisible to asset manager.", virtualPath);
            return CachedAsset<StaticMesh>{};
        }

        return LoadInternal<StaticMesh>(assetMonitor->GetGuid(EAssetType::StaticMesh, virtualPath), *staticMeshLoader);
    }

    Guid AssetManager::CreateMaterial(const String virtualPath, const MaterialCreateDesc createDesc)
    {
        if (!IsValidVirtualPath(virtualPath))
        {
            IG_LOG(AssetManager, Error, "Create Material: Invalid Virtual Path {}", virtualPath);
            return Guid{};
        }

        Result<Material::Desc, EMaterialCreateStatus> result{ materialImporter->Create(AssetInfo{ virtualPath, EAssetType::Material },
                                                                                       createDesc) };
        std::optional<Guid> guidOpt{ ImportInternal<Material>(virtualPath, result) };
        if (!guidOpt)
        {
            return Guid{};
        }

        assetModifiedEvent.Notify(*this);
        return *guidOpt;
    }

    CachedAsset<Material> AssetManager::LoadMaterial(const Guid guid)
    {
        CachedAsset<Material> cachedMat{ LoadInternal<Material>(guid, *materialLoader) };
        if (!cachedMat)
        {
            return LoadInternal<Material>(Guid{ DefaultMaterialGuid }, *materialLoader);
        }

        return cachedMat;
    }

    CachedAsset<Material> AssetManager::LoadMaterial(const String virtualPath)
    {
        if (!IsValidVirtualPath(virtualPath))
        {
            IG_LOG(AssetManager, Error, "Load Material: Invalid Virtual Path {}", virtualPath);
            return LoadInternal<Material>(Guid{ DefaultMaterialGuid }, *materialLoader);
        }

        if (!assetMonitor->Contains(EAssetType::Material, virtualPath))
        {
            IG_LOG(AssetManager, Error, "Material \"{}\" is invisible to asset manager.", virtualPath);
            return LoadInternal<Material>(Guid{ DefaultMaterialGuid }, *materialLoader);
        }

        return LoadMaterial(assetMonitor->GetGuid(EAssetType::Material, virtualPath));
    }

    void AssetManager::Delete(const Guid guid)
    {
        if (!assetMonitor->Contains(guid))
        {
            IG_LOG(AssetManager, Error,
                   "Failed to delete asset. The asset guid (\"{}\") is invisible to asset manager or invalid.",
                   guid.str());
            return;
        }

        DeleteInternal(assetMonitor->GetAssetInfo(guid).GetType(), guid);
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

    AssetInfo AssetManager::GetAssetInfo(const Guid guid) const
    {
        if (!assetMonitor->Contains(guid))
        {
            IG_LOG(AssetManager, Error, "\"{}\" is invisible to asset manager.", guid);
        }

        return assetMonitor->GetAssetInfo(guid);
    }

    std::vector<AssetManager::Snapshot> AssetManager::TakeSnapshots() const
    {
        UnorderedMap<Guid, Snapshot> intermediateSnapshots{};
        std::vector<AssetInfo> assetInfoSnapshots{ assetMonitor->TakeSnapshots() };
        for (const AssetInfo& assetInfoSnapshot : assetInfoSnapshots)
        {
            if (assetInfoSnapshot.IsValid())
            {
                intermediateSnapshots[assetInfoSnapshot.GetGuid()] = Snapshot{ .Info = assetInfoSnapshot, .RefCount = 0 };
            }
        }

        for (const Ptr<details::TypelessAssetCache>& assetCache : assetCaches)
        {
            std::vector<details::TypelessAssetCache::Snapshot> assetCacheSnapshots{ assetCache->TakeSnapshots() };
            for (const details::TypelessAssetCache::Snapshot& cacheSnapshot : assetCacheSnapshots)
            {
                IG_CHECK(intermediateSnapshots.contains(cacheSnapshot.Guid));
                Snapshot& snapshot{ intermediateSnapshots.at(cacheSnapshot.Guid) };
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

    void AssetManager::DeleteInternal(const EAssetType assetType, const Guid guid)
    {
        UniqueLock assetLock{ RequestAssetLock(guid) };
        IG_CHECK(assetType != EAssetType::Unknown);
        IG_CHECK(guid.isValid() && assetMonitor->Contains(guid) && assetMonitor->GetAssetInfo(guid).GetType() == assetType);

        details::TypelessAssetCache& assetCache = GetTypelessCache(assetType);
        if (assetCache.IsCached(guid))
        {
            assetCache.Invalidate(guid);
        }
        assetMonitor->Remove(guid);

        IG_LOG(AssetManager, Info, "Asset \"{}\" deleted.", guid);
        assetModifiedEvent.Notify(*this);
    }

    UniqueLock AssetManager::RequestAssetLock(const Guid guid)
    {
        UniqueLock lock{ assetMutexTableMutex };
        if (!assetMutexTable.contains(guid))
        {
            assetMutexTable[guid];
        }

        return UniqueLock{ assetMutexTable[guid] };
    }

    void AssetManager::SaveAllChanges()
    {
        assetMonitor->SaveAllChanges();
    }
} // namespace ig