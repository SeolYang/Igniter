#pragma once
#include "Igniter/Core/Result.h"
#include "Igniter/Core/Log.h"
#include "Igniter/Core/Event.h"
#include "Igniter/Asset/Common.h"
#include "Igniter/Asset/AssetMonitor.h"
#include "Igniter/Asset/AssetCache.h"
#include "Igniter/Asset/Texture.h"
#include "Igniter/Asset/TextureLoader.h"
#include "Igniter/Asset/StaticMesh.h"
#include "Igniter/Asset/StaticMeshLoader.h"
#include "Igniter/Asset/Material.h"
#include "Igniter/Asset/MaterialLoader.h"
#include "Igniter/Asset/Map.h"
#include "Igniter/Asset/MapLoader.h"

IG_DEFINE_LOG_CATEGORY(AssetManagerLog);

namespace ig::details
{
    enum class EDefaultDummyStatus
    {
        Success,
    };
}

namespace ig
{
    class TextureImporter;
    class TextureLoader;
    class StaticMeshImporter;
    class StaticMeshLoader;
    class MaterialImporter;
    class MaterialLoader;
    class MapCreator;
    class MapLoader;

    class AssetManager final
    {
    private:
        using VirtualPathGuidTable = UnorderedMap<String, Guid>;
        using AssetMutex = Mutex;
        using AssetLock = UniqueLock;

    public:
        using ModifiedEvent = Event<String, std::reference_wrapper<const AssetManager>>;

        struct Snapshot
        {
        public:
            bool IsCached() const { return RefCount > 0 || Info.GetScope() == EAssetScope::Static || Info.GetScope() == EAssetScope::Engine; }

        public:
            AssetInfo Info{};
            uint32_t RefCount{};
        };

    public:
        AssetManager(RenderContext& renderContext);
        AssetManager(const AssetManager&) = delete;
        AssetManager(AssetManager&&) noexcept = delete;
        ~AssetManager();

        AssetManager& operator=(const AssetManager&) = delete;
        AssetManager& operator=(AssetManager&&) noexcept = delete;

        /*
         * #sy_note 에셋 매니저를 사용한 리소스의 로드/언로드
         * 에셋 매니저가 제공하는 로드 함수를 통해 특정 에셋을 로드하면
         * 에셋의 레퍼런스 카운트가 증가함. 즉 반드시 1번의 로드에 최소 1번의 언로드를 매칭 시켜주어야 함.
         */

        Guid Import(const String resPath, const TextureImportDesc& desc);
        [[nodiscard]] ManagedAsset<Texture> LoadTexture(const Guid& guid);
        [[nodiscard]] ManagedAsset<Texture> LoadTexture(const String virtualPath);

        std::vector<Guid> Import(const String resPath, const StaticMeshImportDesc& desc);
        [[nodiscard]] ManagedAsset<StaticMesh> LoadStaticMesh(const Guid& guid);
        [[nodiscard]] ManagedAsset<StaticMesh> LoadStaticMesh(const String virtualPath);

        Guid Import(const String virtualPath, const MaterialAssetCreateDesc& createDesc);
        [[nodiscard]] ManagedAsset<Material> LoadMaterial(const Guid& guid);
        [[nodiscard]] ManagedAsset<Material> LoadMaterial(const String virtualPath);

        Guid Import(const String virtualPath, const MapCreateDesc& desc);
        [[nodiscard]] ManagedAsset<Map> LoadMap(const Guid& guid);
        [[nodiscard]] ManagedAsset<Map> LoadMap(const String virtualPath);

        template <typename T>
        [[nodiscard]] ManagedAsset<T> Load(const Guid& guid)
        {
            if constexpr (AssetCategoryOf<T> == EAssetCategory::Texture)
            {
                return LoadTexture(guid);
            }
            else if constexpr (AssetCategoryOf<T> == EAssetCategory::StaticMesh)
            {
                return LoadStaticMesh(guid);
            }
            else if constexpr (AssetCategoryOf<T> == EAssetCategory::Material)
            {
                return LoadMaterial(guid);
            }
            else if constexpr (AssetCategoryOf<T> == EAssetCategory::Map)
            {
                return LoadMap(guid);
            }
            else
            {
                return {};
            }
        }

        template <typename T>
        [[nodiscard]] ManagedAsset<T> Load(const String virtualPath)
        {
            if constexpr (AssetCategoryOf<T> == EAssetCategory::Texture)
            {
                return LoadTexture(virtualPath);
            }
            else if constexpr (AssetCategoryOf<T> == EAssetCategory::StaticMesh)
            {
                return LoadStaticMesh(virtualPath);
            }
            else if constexpr (AssetCategoryOf<T> == EAssetCategory::Material)
            {
                return LoadMaterial(virtualPath);
            }
            else if constexpr (AssetCategoryOf<T> == EAssetCategory::Map)
            {
                return LoadMap(virtualPath);
            }
        }

        // Reload는 항상 메모리 상에 로드 되어 있는(디스크 상 파일이 아닌)데이터(asset info/description)를 기준으로 한다.
        template <typename T>
        bool Reload(const Guid& guid)
        {
            if (!assetMonitor->Contains(guid))
            {
                IG_LOG(AssetManagerLog, Error, "{} asset \"{}\" is invisible to asset manager.", AssetCategoryOf<T>, guid);
                return false;
            }

            bool bSuceeded = false;
            const typename T::Desc desc{assetMonitor->GetDesc<T>(guid)};
            if constexpr (AssetCategoryOf<T> == EAssetCategory::Texture)
            {
                bSuceeded = ReloadImpl<Texture>(guid, desc, *textureLoader);
            }
            else if constexpr (AssetCategoryOf<T> == EAssetCategory::StaticMesh)
            {
                bSuceeded = ReloadImpl<StaticMesh>(guid, desc, *staticMeshLoader);
            }
            else if constexpr (AssetCategoryOf<T> == EAssetCategory::Material)
            {
                bSuceeded = ReloadImpl<Material>(guid, desc, *materialLoader);
            }
            else if constexpr (AssetCategoryOf<T> == EAssetCategory::Map)
            {
                bSuceeded = ReloadImpl<Map>(guid, desc, *mapLoader);
            }
            else
            {
                IG_CHECK_NO_ENTRY();
                IG_LOG(AssetManagerLog, Error, "Reload Unsupported Asset Type {}.", AssetCategoryOf<T>);
            }

            if (bSuceeded)
            {
                bIsDirty = true;
            }

            return bSuceeded;
        }

        void Delete(const Guid& guid);
        void Delete(const EAssetCategory assetType, const String virtualPath);

        template <typename T>
        T* Lookup(const ManagedAsset<T> handle)
        {
            return GetCache<T>().Lookup(handle);
        }

        template <typename T>
        const T* Lookup(const ManagedAsset<T> handle) const
        {
            return GetCache<T>().Lookup(handle);
        }

        template <typename T>
        void Unload(const ManagedAsset<T> handle)
        {
            /* #sy_todo not thread safe... make it safe! */
            details::AssetCache<T>& cache = GetCache<T>();
            T* ptr = cache.Lookup(handle);
            if (ptr != nullptr)
            {
                const T::Desc& desc = ptr->GetSnapshot();
                AssetLock assetLock{GetAssetMutex(desc.Info.GetGuid())};
                cache.Unload(desc.Info);
            }
        }

        void SaveAllChanges();

        [[nodiscard]] std::optional<AssetInfo> GetAssetInfo(const Guid& guid) const;

        template <typename T>
        [[nodiscard]] std::optional<typename T::LoadDesc> GetLoadDesc(const Guid& guid) const
        {
            if (!assetMonitor->Contains(guid))
            {
                IG_LOG(AssetManagerLog, Error, "\"{}\" is invisible to asset manager.", guid);
                return std::nullopt;
            }

            return assetMonitor->GetLoadDesc<T>(guid);
        }

        template <typename T>
        void UpdateLoadDesc(const Guid& guid, const typename T::LoadDesc& newLoadDesc)
        {
            if (!assetMonitor->Contains(guid))
            {
                IG_LOG(AssetManagerLog, Error, "\"{}\" is invisible to asset manager.", guid);
                return;
            }

            assetMonitor->UpdateLoadDesc<T>(guid, newLoadDesc);
        }

        [[nodiscard]] std::vector<Snapshot> TakeSnapshots() const;

        [[nodiscard]] ModifiedEvent& GetModifiedEvent() { return assetModifiedEvent; }

        void DispatchEvent();

    private:
        template <typename T>
        details::AssetCache<T>& GetCache()
        {
            return static_cast<details::AssetCache<T>&>(GetTypelessCache(AssetCategoryOf<T>));
        }

        template <typename T>
        const details::AssetCache<T>& GetCache() const
        {
            return static_cast<const details::AssetCache<T>&>(GetTypelessCache(AssetCategoryOf<T>));
        }

        details::TypelessAssetCache& GetTypelessCache(const EAssetCategory assetType);
        const details::TypelessAssetCache& GetTypelessCache(const EAssetCategory assetType) const;

        template <typename T, ResultStatus ImportStatus>
        std::optional<Guid> ImportImpl(String resPath, Result<typename T::Desc, ImportStatus>& result)
        {
            constexpr auto AssetType{AssetCategoryOf<T>};
            if (!result.HasOwnership())
            {
                IG_LOG(AssetManagerLog, Error, "{}: Failed({}) to import \"{}\".", AssetType, result.GetStatus(), resPath);

                return std::nullopt;
            }

            const T::Desc desc{result.Take()};
            AssetInfo assetInfo{desc.Info};
            IG_CHECK(assetInfo.IsValid());
            IG_CHECK(assetInfo.GetCategory() == AssetType);

            const String virtualPath{assetInfo.GetVirtualPath()};
            IG_CHECK(IsValidVirtualPath(virtualPath));

            bool bShouldReload{false};
            if (assetMonitor->Contains(AssetType, virtualPath))
            {
                const AssetInfo oldAssetInfo{assetMonitor->GetAssetInfo(AssetType, virtualPath)};
                IG_CHECK(oldAssetInfo.IsValid());
                if (oldAssetInfo.GetScope() == EAssetScope::Engine)
                {
                    IG_LOG(AssetManagerLog, Error, "{}: Failed to import \"{}\". Given virtual path {} was reserved by engine.", AssetType, resPath,
                           virtualPath);

                    return std::nullopt;
                }

                assetMonitor->Remove(oldAssetInfo.GetGuid(), false);

                const Path tempAssetDirPath = GetTempAssetDirectoryPath(AssetType);
                if (!fs::exists(tempAssetDirPath))
                {
                    fs::create_directory(tempAssetDirPath);
                }

                const Path oldAssetPath{MakeAssetPath(AssetType, oldAssetInfo.GetGuid())};
                const Path oldAssetMetadataPath{MakeAssetMetadataPath(AssetType, oldAssetInfo.GetGuid())};
                const Path oldAssetTempPath{MakeTempAssetPath(AssetType, oldAssetInfo.GetGuid())};
                const Path oldAssetMetadataTempPath{MakeTempAssetMetadataPath(AssetType, oldAssetInfo.GetGuid())};

                const bool bShouldTemporalizeAsset = !fs::exists(oldAssetTempPath) && fs::exists(oldAssetPath);
                const bool bShouldTemporalizeMeta = !fs::exists(oldAssetMetadataTempPath) && fs::exists(oldAssetMetadataPath);
                if (bShouldTemporalizeAsset && bShouldTemporalizeMeta)
                {
                    fs::copy(oldAssetPath, oldAssetTempPath);
                    fs::copy(oldAssetMetadataPath, oldAssetMetadataTempPath);
                }
                fs::remove(oldAssetPath);
                fs::remove(oldAssetMetadataPath);

                const Path newAssetPath{MakeAssetPath(AssetType, assetInfo.GetGuid())};
                const Path newAssetMetadataPath{MakeAssetMetadataPath(AssetType, assetInfo.GetGuid())};
                fs::rename(newAssetPath, oldAssetPath);
                fs::rename(newAssetMetadataPath, oldAssetMetadataPath);

                assetInfo.SetGuid(oldAssetInfo.GetGuid());
                bShouldReload = true;
            }

            assetMonitor->Create<T>(assetInfo, desc.LoadDescriptor);
            IG_LOG(AssetManagerLog, Info, "{}: \"{}\" imported as {} ({}).", AssetType, resPath, virtualPath, assetInfo.GetGuid());

            if (bShouldReload)
            {
                Reload<T>(assetInfo.GetGuid());
            }

            return assetInfo.GetGuid();
        }

        template <typename T, typename AssetLoader>
        [[nodiscard]] ManagedAsset<T> LoadImpl(const Guid& guid, AssetLoader& loader)
        {
            if (!assetMonitor->Contains(guid))
            {
                IG_LOG(AssetManagerLog, Error, "{} asset \"{}\" is invisible to asset manager.", AssetCategoryOf<T>, guid);
                return ManagedAsset<T>{};
            }

            AssetLock assetLock{GetAssetMutex(guid)};
            details::AssetCache<T>& assetCache{GetCache<T>()};
            if (!assetCache.IsCached(guid))
            {
                const T::Desc desc{assetMonitor->GetDesc<T>(guid)};
                IG_CHECK(desc.Info.GetGuid() == guid);
                auto result{loader.Load(desc)};
                if (!result.HasOwnership())
                {
                    IG_LOG(AssetManagerLog, Error, "Failed({}) to load {} asset {} ({}).", AssetCategoryOf<T>, result.GetStatus(),
                           desc.Info.GetVirtualPath(), guid);
                    return ManagedAsset<T>{};
                }

                assetCache.Cache(guid, result.Take());
                IG_LOG(AssetManagerLog, Info, "{} asset {} ({}) cached.", AssetCategoryOf<T>, desc.Info.GetVirtualPath(), guid);
            }

            IG_LOG(AssetManagerLog, Info, "Cache Hit! {} asset {} loaded.", AssetCategoryOf<T>, guid);
            bIsDirty = true;
            return assetCache.Load(guid);
        }

        template <typename T, typename AssetLoader>
        bool ReloadImpl(const Guid& guid, const typename T::Desc& desc, AssetLoader& loader)
        {
            /* #sy_note
             * 현재 구현은 핸들이 가르키는 메모리 공간의 인스턴스를 replace 하는 방식으로 구현
             * 이 경우, 해당 메모리 공간에 대한 스레드 안전성은 보장 되지 않음.
             * 이후, 해당 메모리 공간에 대한 스레드 안전성을 보장하기 위해서는 이전 핸들을 invalidate 시키고
             * 해당 핸들을 참조하는 측에서 별도의 fallback (예시. 1차 fallback => 캐시된 guid로 재 로드 시도, 2차 fallback => 엔진 기본 에셋)
             * 을 제공하는 방식으로 구현하는 것이 좋아 보임.
             */
            IG_CHECK(assetMonitor->Contains(guid));
            IG_CHECK(desc.Info.GetGuid() == guid);
            details::AssetCache<T>& assetCache{GetCache<T>()};
            AssetLock assetLock{GetAssetMutex(guid)};
            if (!assetCache.IsCached(guid))
            {
                return false;
            }

            auto result{loader.Load(desc)};
            if (!result.HasOwnership())
            {
                IG_LOG(AssetManagerLog, Error, "{} asset \"{}\" failed to reload.", AssetCategoryOf<T>, guid);
                return false;
            }

            ManagedAsset<T> cachedAsset{assetCache.Load(guid, false)};
            if (cachedAsset)
            {
                T* cachedAssetPtr = assetCache.Lookup(cachedAsset);
                IG_CHECK(cachedAssetPtr != nullptr);
                *cachedAssetPtr = result.Take();
            }
            else
            {
                assetCache.Cache(guid, result.Take());
            }

            return true;
        }

        void DeleteImpl(const EAssetCategory assetType, const Guid& guid);

        [[nodiscard]] AssetMutex& GetAssetMutex(const Guid& guid);

        template <typename T, ResultStatus Status>
        void RegisterEngineInternalAsset(const String requiredVirtualPath, Result<T, Status> assetResult)
        {
            if (!assetResult.HasOwnership())
            {
                IG_LOG(AssetManagerLog, Error, "{}: Failed({}) to create engine default asset {}.", AssetCategoryOf<T>, assetResult.GetStatus(),
                       requiredVirtualPath);
                return;
            }

            T asset{assetResult.Take()};
            const typename T::Desc& desc{asset.GetSnapshot()};
            const AssetInfo& assetInfo{desc.Info};
            IG_CHECK(assetInfo.IsValid());
            IG_CHECK(assetInfo.GetCategory() == AssetCategoryOf<T>);
            IG_CHECK(assetInfo.GetScope() == EAssetScope::Engine);

            /* #sy_note GUID를 고정으로 할지 말지.. 일단 Virtual Path는 고정적이고, Import 되고난 이후엔 Virtual Path로 접근 가능하니 유동적으로 */
            Result<typename T::Desc, details::EDefaultDummyStatus> dummyResult{MakeSuccess<typename T::Desc, details::EDefaultDummyStatus>(desc)};
            IG_CHECK(dummyResult.HasOwnership());
            std::optional<Guid> guidOpt{ImportImpl<T>(assetInfo.GetVirtualPath(), dummyResult)};
            IG_CHECK(guidOpt);

            const Guid& guid{*guidOpt};
            IG_CHECK(guid.isValid());
            AssetLock assetLock{GetAssetMutex(guid)};
            details::AssetCache<T>& assetCache{GetCache<T>()};
            IG_CHECK(!assetCache.IsCached(guid));
            assetCache.Cache(guid, std::move(asset));
        }

        void RegisterEngineDefault();
        void UnRegisterEngineDefault();

        void ClearTempAssets();
        void RestoreTempAssets();

    private:
        Ptr<details::AssetMonitor> assetMonitor;
        std::vector<Ptr<details::TypelessAssetCache>> assetCaches;

        Mutex assetMutexTableMutex;
        StableUnorderedMap<Guid, AssetMutex> assetMutexTable;

        Ptr<TextureImporter> textureImporter;
        Ptr<TextureLoader> textureLoader;

        Ptr<StaticMeshImporter> staticMeshImporter;
        Ptr<StaticMeshLoader> staticMeshLoader;

        Ptr<MaterialImporter> materialImporter;
        Ptr<MaterialLoader> materialLoader;

        Ptr<MapCreator> mapCreator;
        Ptr<MapLoader> mapLoader;

        bool bIsDirty = false;
        ModifiedEvent assetModifiedEvent;
    };
} // namespace ig
