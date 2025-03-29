#include "Igniter/Igniter.h"
#include "Igniter/Asset/TextureImporter.h"
#include "Igniter/Asset/StaticMeshImporter.h"
#include "Igniter/Asset/MaterialImporter.h"
#include "Igniter/Asset/MapCreator.h"
#include "Igniter/Asset/AudioClipImporter.h"
#include "Igniter/Asset/AssetManager.h"

IG_DEFINE_LOG_CATEGORY(AssetManagerLog);

namespace ig
{
    AssetManager::AssetManager(RenderContext& renderContext, AudioSystem& audioSystem)
        : textureImporter(MakePtr<TextureImporter>())
        , textureLoader(MakePtr<TextureLoader>(renderContext))
        , staticMeshImporter(MakePtr<StaticMeshImporter>(*this))
        , staticMeshLoader(MakePtr<StaticMeshLoader>(renderContext, *this))
        , materialImporter(MakePtr<MaterialImporter>(*this))
        , materialLoader(MakePtr<MaterialLoader>(*this))
        , audioImporter(MakePtr<AudioClipImporter>())
        , audioLoader(MakePtr<AudioClipLoader>(audioSystem))
    {
        RestoreTempAssets();
        assetMonitor = MakePtr<details::AssetMonitor>();

        assetCaches.emplace_back(MakePtr<details::AssetCache<Texture>>());
        assetCaches.emplace_back(MakePtr<details::AssetCache<StaticMesh>>());
        assetCaches.emplace_back(MakePtr<details::AssetCache<Material>>());
        assetCaches.emplace_back(MakePtr<details::AssetCache<Map>>());
        assetCaches.emplace_back(MakePtr<details::AssetCache<AudioClip>>());
        RegisterEngineDefault();
    }

    AssetManager::~AssetManager()
    {
        UnRegisterEngineDefault();
        for (const auto& snapshot : TakeSnapshots())
        {
            if (snapshot.IsCached())
            {
                IG_LOG(AssetManagerLog,
                    Fatal,
                    "Asset {} still cached(alived) with ref count: {}. Please Checks Load-Unload call pair.", snapshot.Info,
                    snapshot.RefCount);
            }
        }

        RestoreTempAssets();
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
        IG_LOG(AssetManagerLog, Info, "Load and register Engine Default Assets to Asset Manager.");

        /* #sy_wip 기본 에셋 Virtual Path들도 AssetManager 컴파일 타임 상수로 통합 */
        const AssetInfo defaultTexInfo{Guid{DefaultTextureGuid}, Texture::EngineDefault, EAssetCategory::Texture, EAssetScope::Engine};
        RegisterEngineInternalAsset<Texture>(defaultTexInfo.GetVirtualPath(), textureLoader->MakeDefault(defaultTexInfo));

        const AssetInfo defaultWhiteTexInfo{Guid{DefaultWhiteTextureGuid}, Texture::EngineDefaultWhite, EAssetCategory::Texture, EAssetScope::Engine};
        RegisterEngineInternalAsset<Texture>(
            defaultWhiteTexInfo.GetVirtualPath(), textureLoader->MakeMonochrome(defaultWhiteTexInfo, Color{1.f, 1.f, 1.f}));

        const AssetInfo defaultBlackTexInfo{Guid{DefaultBlackTextureGuid}, Texture::EngineDefaultBlack, EAssetCategory::Texture, EAssetScope::Engine};
        RegisterEngineInternalAsset<Texture>(
            defaultBlackTexInfo.GetVirtualPath(), textureLoader->MakeMonochrome(defaultBlackTexInfo, Color{0.f, 0.f, 0.f}));

        const AssetInfo defaultMatInfo{Guid{DefaultMaterialGuid}, Material::EngineDefault, EAssetCategory::Material, EAssetScope::Engine};
        RegisterEngineInternalAsset<Material>(defaultMatInfo.GetVirtualPath(), materialLoader->MakeDefault(defaultMatInfo));

        IG_LOG(AssetManagerLog, Info, "Engine Default Assets have been successfully registered to Asset Manager.");
    }

    void AssetManager::UnRegisterEngineDefault()
    {
        IG_LOG(AssetManagerLog, Info, "Unload/Unregister Engine Default Assets from Asset Manager.");

        Delete(Guid{DefaultTextureGuid});
        Delete(Guid{DefaultWhiteTextureGuid});
        Delete(Guid{DefaultBlackTextureGuid});
        Delete(Guid{DefaultMaterialGuid});

        IG_LOG(AssetManagerLog, Info, "Engine Default Assets have been successfully unregistered from Asset Manager.");
    }

    Guid AssetManager::Import(const std::string_view resPath, const TextureImportDesc& desc, const bool bShouldSuppressDirty)
    {
        Result<Texture::Desc, ETextureImportStatus> result = textureImporter->Import(resPath, desc);
        const std::optional<Guid> guidOpt{ImportImpl<Texture>(resPath, result, bShouldSuppressDirty)};
        if (!guidOpt)
        {
            return Guid{};
        }

        return *guidOpt;
    }

    Handle<Texture> AssetManager::LoadTexture(const Guid& guid, const bool bShouldSuppressDirty)
    {
        Handle<Texture> cachedTex{LoadImpl<Texture>(guid, *textureLoader, bShouldSuppressDirty)};
        if (!cachedTex)
        {
            return LoadImpl<Texture>(Guid{DefaultTextureGuid}, *textureLoader, bShouldSuppressDirty);
        }

        return cachedTex;
    }

    Handle<Texture> AssetManager::LoadTexture(const std::string_view virtualPath, const bool bShouldSuppressDirty)
    {
        if (!IsValidVirtualPath(virtualPath))
        {
            IG_LOG(AssetManagerLog, Error, "Load Texture: Invalid Virtual Path {}", virtualPath);
            return LoadImpl<Texture>(Guid{DefaultTextureGuid}, *textureLoader, bShouldSuppressDirty);
        }

        if (!assetMonitor->Contains(EAssetCategory::Texture, virtualPath))
        {
            IG_LOG(AssetManagerLog, Error, "Texture \"{}\" is invisible to asset manager.", virtualPath);
            return LoadImpl<Texture>(Guid{DefaultTextureGuid}, *textureLoader, bShouldSuppressDirty);
        }

        return LoadTexture(assetMonitor->GetGuid(EAssetCategory::Texture, virtualPath));
    }

    Vector<Guid> AssetManager::Import(const std::string_view resPath, const StaticMeshImportDesc& desc, const bool bShouldSuppressDirty)
    {
        Vector<Result<StaticMesh::Desc, EStaticMeshImportStatus>> results = staticMeshImporter->Import(resPath, desc);
        Vector<Guid> output;
        output.reserve(results.size());
        for (Result<StaticMesh::Desc, EStaticMeshImportStatus>& result : results)
        {
            std::optional<Guid> guidOpt{ImportImpl<StaticMesh>(resPath, result, bShouldSuppressDirty)};
            if (guidOpt)
            {
                output.emplace_back(*guidOpt);
            }
        }

        return output;
    }

    Handle<StaticMesh> AssetManager::LoadStaticMesh(const Guid& guid, const bool bShouldSuppressDirty)
    {
        return LoadImpl<StaticMesh>(guid, *staticMeshLoader, bShouldSuppressDirty);
    }

    Handle<StaticMesh> AssetManager::LoadStaticMesh(const std::string_view virtualPath, const bool bShouldSuppressDirty)
    {
        if (!IsValidVirtualPath(virtualPath))
        {
            IG_LOG(AssetManagerLog, Error, "Load Static Mesh: Invalid Virtual Path {}", virtualPath);
            return Handle<StaticMesh>{};
        }

        if (!assetMonitor->Contains(EAssetCategory::StaticMesh, virtualPath))
        {
            IG_LOG(AssetManagerLog, Error, "Static mesh \"{}\" is invisible to asset manager.", virtualPath);
            return Handle<StaticMesh>{};
        }

        return LoadImpl<StaticMesh>(assetMonitor->GetGuid(EAssetCategory::StaticMesh, virtualPath), *staticMeshLoader, bShouldSuppressDirty);
    }

    Guid AssetManager::Create(const std::string_view virtualPath, const MaterialAssetCreateDesc& createDesc, const bool bShouldSuppressDirty)
    {
        if (!IsValidVirtualPath(virtualPath))
        {
            IG_LOG(AssetManagerLog, Error, "Import Material: Invalid Virtual Path {}", virtualPath);
            return Guid{};
        }

        Result<Material::Desc, EMaterialAssetImportStatus> result{materialImporter->Import(AssetInfo{virtualPath, EAssetCategory::Material}, createDesc)};
        std::optional<Guid> guidOpt{ImportImpl<Material>(virtualPath, result, bShouldSuppressDirty)};
        if (!guidOpt)
        {
            return Guid{};
        }

        return *guidOpt;
    }

    Handle<Material> AssetManager::LoadMaterial(const Guid& guid, const bool bShouldSuppressDirty)
    {
        Handle<Material> cachedMat{LoadImpl<Material>(guid, *materialLoader, bShouldSuppressDirty)};
        if (!cachedMat)
        {
            return LoadImpl<Material>(Guid{DefaultMaterialGuid}, *materialLoader, bShouldSuppressDirty);
        }

        return cachedMat;
    }

    Handle<Material> AssetManager::LoadMaterial(const std::string_view virtualPath, const bool bShouldSuppressDirty)
    {
        if (!IsValidVirtualPath(virtualPath))
        {
            IG_LOG(AssetManagerLog, Error, "Load Material: Invalid Virtual Path {}", virtualPath);
            return LoadImpl<Material>(Guid{DefaultMaterialGuid}, *materialLoader, bShouldSuppressDirty);
        }

        if (!assetMonitor->Contains(EAssetCategory::Material, virtualPath))
        {
            IG_LOG(AssetManagerLog, Error, "Material \"{}\" is invisible to asset manager.", virtualPath);
            return LoadImpl<Material>(Guid{DefaultMaterialGuid}, *materialLoader, bShouldSuppressDirty);
        }

        return LoadMaterial(assetMonitor->GetGuid(EAssetCategory::Material, virtualPath));
    }

    Guid AssetManager::Create(const std::string_view virtualPath, const MapCreateDesc& desc, const bool bShouldSuppressDirty)
    {
        if (!IsValidVirtualPath(virtualPath))
        {
            IG_LOG(AssetManagerLog, Error, "Failed to create map: Invalid Virtual Path {}", virtualPath);
            return Guid{};
        }

        Result<Map::Desc, EMapCreateStatus> result{mapCreator->Import(AssetInfo{virtualPath, EAssetCategory::Map}, desc)};
        std::optional<Guid> guidOpt{ImportImpl<Map>(virtualPath, result, bShouldSuppressDirty)};
        if (!guidOpt)
        {
            return Guid{};
        }

        return *guidOpt;
    }

    Handle<Map> AssetManager::LoadMap(const Guid& guid, const bool bShouldSuppressDirty)
    {
        Handle<Map> cachedMap{LoadImpl<Map>(guid, *mapLoader, bShouldSuppressDirty)};
        if (!cachedMap)
        {
            IG_LOG(AssetManagerLog, Error, "Failed to load map {}.", guid);
        }

        return cachedMap;
    }

    Handle<Map> AssetManager::LoadMap(const std::string_view virtualPath, const bool bShouldSuppressDirty)
    {
        if (!IsValidVirtualPath(virtualPath))
        {
            IG_LOG(AssetManagerLog, Error, "Load Map: Invalid Virtual Path {}", virtualPath);
            return {};
        }

        if (!assetMonitor->Contains(EAssetCategory::Map, virtualPath))
        {
            IG_LOG(AssetManagerLog, Error, "Map \"{}\" is invisible to asset manager.", virtualPath);
            return {};
        }

        return LoadMap(assetMonitor->GetGuid(EAssetCategory::Map, virtualPath), bShouldSuppressDirty);
    }

    Guid AssetManager::Import(const std::string_view resPath, const AudioClipImportDesc& desc, const bool bShouldSuppressDirty)
    {
        Result<AudioClip::Desc, EAudioClipImportError> result{audioImporter->Import(desc)};
        const std::optional<Guid> guidOpt{ImportImpl<AudioClip>(resPath, result, bShouldSuppressDirty)};
        if (!guidOpt)
        {
            return Guid{};
        }

        return *guidOpt;
    }
    
    Handle<AudioClip> AssetManager::LoadAudioClip(const Guid& guid, const bool bShouldSuppressDirty)
    {
        const Handle<AudioClip> cachedAudioClipHandle{LoadImpl<AudioClip>(guid, *audioLoader, bShouldSuppressDirty)};
        if (!cachedAudioClipHandle)
        {
            IG_LOG(AssetManagerLog, Error, "Failed to load audio clip {}.", guid);
        }

        return cachedAudioClipHandle;
    }
    
    Handle<AudioClip> AssetManager::LoadAudioClip(const std::string_view virtualPath, const bool bShouldSuppressDirty)
    {
        if (!IsValidVirtualPath(virtualPath))
        {
            IG_LOG(AssetManagerLog, Error, "Load Audio Clip: Invalid Virtual Path {}", virtualPath);
            return {};
        }

        if (!assetMonitor->Contains(EAssetCategory::Audio, virtualPath))
        {
            IG_LOG(AssetManagerLog, Error, "Audio Clip \"{}\" is invisible to asset manager.", virtualPath);
            return {};
        }

        return LoadAudioClip(assetMonitor->GetGuid(EAssetCategory::Audio, virtualPath), bShouldSuppressDirty);
    }

    void AssetManager::Delete(const Guid& guid, const bool bShouldSuppressDirty)
    {
        if (!assetMonitor->Contains(guid))
        {
            IG_LOG(AssetManagerLog, Error, "Failed to delete asset. The asset guid (\"{}\") is invisible to asset manager or invalid.", guid.str());
            return;
        }

        DeleteImpl(assetMonitor->GetAssetInfo(guid).GetCategory(), guid, bShouldSuppressDirty);
    }

    void AssetManager::Delete(const EAssetCategory assetType, const std::string_view virtualPath, const bool bShouldSuppressDirty)
    {
        if (assetType == EAssetCategory::Unknown)
        {
            IG_LOG(AssetManagerLog, Error, "Failed to delete asset. The asset type is unknown.");
            return;
        }

        if (!assetMonitor->Contains(assetType, virtualPath))
        {
            IG_LOG(AssetManagerLog, Error, "Failed to delete asset. The virtual path is invisible to asset manager or invalid.");
            return;
        }

        DeleteImpl(assetType, assetMonitor->GetGuid(assetType, virtualPath), bShouldSuppressDirty);
    }

    void AssetManager::DeleteImpl(const EAssetCategory assetType, const Guid& guid, const bool bShouldSuppressDirty)
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

        IG_LOG(AssetManagerLog, Info, "Asset \"{}\" deleted.", guid);

        if (!bShouldSuppressDirty)
        {
            bIsDirty = true;
        }
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
        ClearTempAssets();
    }

    std::optional<AssetInfo> AssetManager::GetAssetInfo(const Guid& guid) const
    {
        if (!assetMonitor->Contains(guid))
        {
            IG_LOG(AssetManagerLog, Error, "\"{}\" is invisible to asset manager.", guid);
            return std::nullopt;
        }

        return assetMonitor->GetAssetInfo(guid);
    }

    Vector<AssetManager::Snapshot> AssetManager::TakeSnapshots(const EAssetCategory filter, const bool bOnlyTakeCached) const
    {
        Vector<Snapshot> snapshots;
        Vector<AssetInfo> assetInfoSnapshots{assetMonitor->TakeSnapshots(filter)};
        snapshots.reserve(assetInfoSnapshots.size());

        if (filter != EAssetCategory::Unknown)
        {
            const details::TypelessAssetCache* assetCachePtr = nullptr;
            for (const Ptr<details::TypelessAssetCache>& assetCache : assetCaches)
            {
                if (assetCache->GetAssetType() == filter)
                {
                    assetCachePtr = assetCache.get();
                    break;
                }
            }

            IG_CHECK(assetCachePtr != nullptr);
            if (assetCachePtr != nullptr)
            {
                for (const AssetInfo& assetInfo : assetInfoSnapshots)
                {
                    const details::TypelessAssetCache::Snapshot cacheSnapshot = assetCachePtr->TakeSnapshot(assetInfo.GetGuid());
                    const Snapshot snapshot{
                        assetInfo,
                        cacheSnapshot.RefCount,
                        cacheSnapshot.HandleHash
                    };

                    if (!bOnlyTakeCached || snapshot.IsCached())
                    {
                        snapshots.emplace_back(snapshot);
                    }
                }
            }
        }
        else
        {
            for (const AssetInfo& assetInfo : assetInfoSnapshots)
            {
                for (const Ptr<details::TypelessAssetCache>& assetCache : assetCaches)
                {
                    if (assetInfo.GetCategory() == assetCache->GetAssetType())
                    {
                        const details::TypelessAssetCache::Snapshot cacheSnapshot = assetCache->TakeSnapshot(assetInfo.GetGuid());
                        snapshots.emplace_back(assetInfo,
                            cacheSnapshot.RefCount,
                            cacheSnapshot.HandleHash);
                        break;
                    }
                }
            }
        }

        return snapshots;
    }

    void AssetManager::DispatchEvent()
    {
        if (bIsDirty.exchange(false))
        {
            assetModifiedEvent.Notify(*this);
        }
    }

    void AssetManager::ClearTempAssets()
    {
        for (auto category : magic_enum::enum_values<EAssetCategory>())
        {
            if (category == EAssetCategory::Unknown)
            {
                continue;
            }

            std::error_code errorCode{};
            const U64 numRemovedFiles = fs::remove_all(GetTempAssetDirectoryPath(category), errorCode);
            if (!errorCode)
            {
                IG_LOG(AssetManagerLog, Info, "Successfully removed {} temp {} assets.", numRemovedFiles, category);
            }
            else
            {
                IG_LOG(AssetManagerLog, Warning, "Failed to remove temp {} assets: {}", category, errorCode.message());
            }
        }
    }

    void AssetManager::RestoreTempAssets()
    {
        for (auto category : magic_enum::enum_values<EAssetCategory>())
        {
            if (category == EAssetCategory::Unknown)
            {
                continue;
            }

            const Path tempAssetDirPath = GetTempAssetDirectoryPath(category);
            if (!fs::exists(tempAssetDirPath))
            {
                continue;
            }

            std::error_code errorCode{};
            fs::copy(GetTempAssetDirectoryPath(category), GetAssetDirectoryPath(category),
                fs::copy_options::recursive | fs::copy_options::overwrite_existing,
                errorCode);

            if (!errorCode)
            {
                IG_LOG(AssetManagerLog, Info, "Successfully restored temp {} assets.", category);
            }
            else
            {
                IG_LOG(AssetManagerLog, Warning, "Failed to restore temp {} assets: {}", category, errorCode.message());
            }

            fs::remove_all(tempAssetDirPath);
        }
    }
} // namespace ig
