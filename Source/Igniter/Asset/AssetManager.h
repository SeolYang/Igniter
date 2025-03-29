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
#include "Igniter/Asset/AudioClip.h"
#include "Igniter/Asset/AudioClipLoader.h"

IG_DECLARE_LOG_CATEGORY(AssetManagerLog);

namespace ig::details
{
    enum class EDefaultDummyStatus
    {
        Success,
    };
}

namespace ig
{
    class RenderContext;
    class AudioSystem;
    class TextureImporter;
    class TextureLoader;
    class StaticMeshImporter;
    class StaticMeshLoader;
    class MaterialImporter;
    class MaterialLoader;
    class MapCreator;
    class MapLoader;
    class AudioClipImporter;
    class AudioClipLoader;

    // #sy_todo bIsSuppress 같은걸 flag로 관리 하기
    // ex. EAssetManagerOptionFlag, SuppressLog, SuppressDirty etc..
    class AssetManager final
    {
    private:
        using VirtualPathGuidTable = UnorderedMap<U64, Guid>;
        using AssetMutex = Mutex;
        using AssetLock = UniqueLock;

    public:
        using ModifiedEvent = Event<std::string, std::reference_wrapper<const AssetManager>>;

        struct Snapshot
        {
        public:
            [[nodiscard]] bool IsCached() const noexcept { return RefCount > 0 || Info.GetScope() == EAssetScope::Static || Info.GetScope() == EAssetScope::Engine; }

        public:
            AssetInfo Info{};
            U32 RefCount{};
            Size HandleHash{IG_NUMERIC_MAX_OF(HandleHash)};
        };

    public:
        explicit AssetManager(RenderContext& renderContext, AudioSystem& audioSystem);
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

        Guid Import(const std::string_view resPath, const TextureImportDesc& desc, const bool bShouldSuppressDirty = false);
        [[nodiscard]] Handle<Texture> LoadTexture(const Guid& guid, const bool bShouldSuppressDirty = false);
        [[nodiscard]] Handle<Texture> LoadTexture(const std::string_view virtualPath, const bool bShouldSuppressDirty = false);

        Vector<Guid> Import(const std::string_view resPath, const StaticMeshImportDesc& desc, const bool bShouldSuppressDirty = false);
        [[nodiscard]] Handle<StaticMesh> LoadStaticMesh(const Guid& guid, const bool bShouldSuppressDirty = false);
        [[nodiscard]] Handle<StaticMesh> LoadStaticMesh(const std::string_view virtualPath, const bool bShouldSuppressDirty = false);

        Guid Create(const std::string_view virtualPath, const MaterialAssetCreateDesc& createDesc, const bool bShouldSuppressDirty = false);
        [[nodiscard]] Handle<Material> LoadMaterial(const Guid& guid, const bool bShouldSuppressDirty = false);
        [[nodiscard]] Handle<Material> LoadMaterial(const std::string_view virtualPath, const bool bShouldSuppressDirty = false);

        Guid Create(const std::string_view virtualPath, const MapCreateDesc& desc, const bool bShouldSuppressDirty = false);
        [[nodiscard]] Handle<Map> LoadMap(const Guid& guid, const bool bShouldSuppressDirty = false);
        [[nodiscard]] Handle<Map> LoadMap(const std::string_view virtualPath, const bool bShouldSuppressDirty = false);

        Guid Import(const std::string_view resPath, const AudioClipImportDesc& desc, const bool bShouldSuppressDirty = false);
        [[nodiscard]] Handle<AudioClip> LoadAudioClip(const Guid& guid, const bool bShouldSuppressDirty = false);
        [[nodiscard]] Handle<AudioClip> LoadAudioClip(const std::string_view virtualPath, const bool bShouldSuppressDirty = false);

        template <typename T>
        [[nodiscard]] Handle<T> Load(const Guid& guid, const bool bShouldSuppressDirty = false)
        {
            if constexpr (AssetCategoryOf<T> == EAssetCategory::Texture)
            {
                return LoadTexture(guid, bShouldSuppressDirty);
            }
            else if constexpr (AssetCategoryOf<T> == EAssetCategory::StaticMesh)
            {
                return LoadStaticMesh(guid, bShouldSuppressDirty);
            }
            else if constexpr (AssetCategoryOf<T> == EAssetCategory::Material)
            {
                return LoadMaterial(guid, bShouldSuppressDirty);
            }
            else if constexpr (AssetCategoryOf<T> == EAssetCategory::Map)
            {
                return LoadMap(guid, bShouldSuppressDirty);
            }
            else if constexpr (AssetCategoryOf<T> == EAssetCategory::Audio)
            {
                return LoadAudioClip(guid, bShouldSuppressDirty);
            }
            else
            {
                return {};
            }
        }

        template <typename T>
        [[nodiscard]] Handle<T> Load(const std::string_view virtualPath, const bool bShouldSuppressDirty = false)
        {
            if constexpr (AssetCategoryOf<T> == EAssetCategory::Texture)
            {
                return LoadTexture(virtualPath, bShouldSuppressDirty);
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
            else if constexpr (AssetCategoryOf<T> == EAssetCategory::Audio)
            {
                return LoadAudioClip(virtualPath);
            }
            else
            {
                return {};
            }
        }

        // Reload는 항상 메모리 상에 로드 되어 있는(디스크 상 파일이 아닌)데이터(asset info/description)를 기준으로 한다.
        template <typename T>
        bool Reload(const Guid& guid, const bool bShouldSuppressDirty = false)
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
                bSuceeded = ReloadImpl<Texture>(guid, desc, *textureLoader, bShouldSuppressDirty);
            }
            else if constexpr (AssetCategoryOf<T> == EAssetCategory::StaticMesh)
            {
                bSuceeded = ReloadImpl<StaticMesh>(guid, desc, *staticMeshLoader, bShouldSuppressDirty);
            }
            else if constexpr (AssetCategoryOf<T> == EAssetCategory::Material)
            {
                bSuceeded = ReloadImpl<Material>(guid, desc, *materialLoader, bShouldSuppressDirty);
            }
            else if constexpr (AssetCategoryOf<T> == EAssetCategory::Map)
            {
                bSuceeded = ReloadImpl<Map>(guid, desc, *mapLoader, bShouldSuppressDirty);
            }
            else if constexpr (AssetCategoryOf<T> == EAssetCategory::Audio)
            {
                bSuceeded = ReloadImpl<AudioClip>(guid, desc, *audioLoader, bShouldSuppressDirty);
            }
            else
            {
                IG_CHECK_NO_ENTRY();
                IG_LOG(AssetManagerLog, Error, "Reload Unsupported Asset Type {}.", AssetCategoryOf<T>);
            }

            return bSuceeded;
        }

        void Delete(const Guid& guid, const bool bShouldSuppressDirty = false);
        void Delete(const EAssetCategory assetType, const std::string_view virtualPath, const bool bShouldSuppressDirty = false);

        template <typename T>
        bool Clone(const Handle<T> handle, const U32 numClones = 1, const bool bShouldSuppressDirty = false)
        {
            if (!handle)
            {
                return false;
            }

            details::AssetCache<T>& cache = GetCache<T>();
            const T* asset = cache.Lookup(handle);
            if (asset == nullptr)
            {
                return false;
            }

            if (!bShouldSuppressDirty)
            {
                bIsDirty = true;
            }

            const AssetInfo& assetInfo = asset->GetSnapshot().Info;
            cache.Clone(assetInfo.GetGuid(), numClones);
            IG_LOG(AssetManagerLog, Info, "<{}:{}> {}({}) cloned({}).",
                assetInfo.GetCategory(), assetInfo.GetScope(),
                assetInfo.GetVirtualPath(), assetInfo.GetGuid(),
                numClones);

            return true;
        }

        template <typename T>
        T* Lookup(const Handle<T> handle)
        {
            return GetCache<T>().Lookup(handle);
        }

        template <typename T>
        const T* Lookup(const Handle<T> handle) const
        {
            return GetCache<T>().Lookup(handle);
        }

        template <typename T>
        void Unload(const Handle<T> handle, const bool bShouldSuppressDirty = false)
        {
            /* #sy_todo not thread safe... make it safe! */
            details::AssetCache<T>& cache = GetCache<T>();
            T* ptr = cache.Lookup(handle);
            if (ptr != nullptr)
            {
                const typename T::Desc desc = ptr->GetSnapshot();
                AssetLock assetLock{GetAssetMutex(desc.Info.GetGuid())};
                cache.Unload(desc.Info);

                if (!bShouldSuppressDirty)
                {
                    bIsDirty = true;
                }
            }
        }

        void SaveAllChanges();

        [[nodiscard]] std::optional<AssetInfo> GetAssetInfo(const Guid& guid) const;

        /*
         * 에셋 인스턴스 내부에 저장되어있는 'Snapshot'은, 인스턴스가 로딩(또는 리로딩) 된 시점에서의 에셋 정보를 담고 있다.
         * 만약, 최신 정보를 얻고싶다면 'AssetManager'를 통해 데이터를 얻어와야만 한다.
         * 마찬가지로, 기본적으로 에셋 인스턴스 내부에 있는 'Snapshot'은 상수로 취급되어야 한다.
         * 그에따라, 에셋 매니저의 최신 데이터를 갱신 시키기 위해선 UploadLoadDesc를 사용하여야 한다. 
         */
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
        void UpdateLoadDesc(const Guid& guid, const typename T::LoadDesc& newLoadDesc, const bool bShouldSuppressDirty = false)
        {
            if (!assetMonitor->Contains(guid))
            {
                IG_LOG(AssetManagerLog, Error, "\"{}\" is invisible to asset manager.", guid);
                return;
            }

            assetMonitor->UpdateLoadDesc<T>(guid, newLoadDesc);
            if (bShouldSuppressDirty)
            {
                bIsDirty = true;
            }
        }

        // Unknown == no filter
        [[nodiscard]] Vector<Snapshot> TakeSnapshots(const EAssetCategory filter = EAssetCategory::Unknown, const bool bOnlyTakeCached = false) const;

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
        std::optional<Guid> ImportImpl(std::string_view resPath, Result<typename T::Desc, ImportStatus>& result, const bool bShouldSuppressDirty)
        {
            constexpr auto kAssetCategory{AssetCategoryOf<T>};
            if (!result.HasOwnership())
            {
                IG_LOG(AssetManagerLog, Error, "{}: Failed({}) to import \"{}\".", kAssetCategory, result.GetStatus(), resPath);

                return std::nullopt;
            }

            const T::Desc desc{result.Take()};
            AssetInfo assetInfo{desc.Info};
            IG_CHECK(assetInfo.IsValid());
            IG_CHECK(assetInfo.GetCategory() == AssetType);

            const std::string_view virtualPath{assetInfo.GetVirtualPath()};
            IG_CHECK(IsValidVirtualPath(virtualPath));

            bool bShouldReload{false};
            if (assetMonitor->Contains(kAssetCategory, virtualPath))
            {
                const AssetInfo oldAssetInfo{assetMonitor->GetAssetInfo(kAssetCategory, virtualPath)};
                IG_CHECK(oldAssetInfo.IsValid());
                if (oldAssetInfo.GetScope() == EAssetScope::Engine)
                {
                    IG_LOG(AssetManagerLog, Error, "{}: Failed to import \"{}\". Given virtual path {} was reserved by engine.", kAssetCategory, resPath,
                        virtualPath);

                    return std::nullopt;
                }

                assetMonitor->Remove(oldAssetInfo.GetGuid(), false);

                const Path tempAssetDirPath = GetTempAssetDirectoryPath(kAssetCategory);
                if (!fs::exists(tempAssetDirPath))
                {
                    fs::create_directory(tempAssetDirPath);
                }

                const Path oldAssetPath{MakeAssetPath(kAssetCategory, oldAssetInfo.GetGuid())};
                const Path oldAssetMetadataPath{MakeAssetMetadataPath(kAssetCategory, oldAssetInfo.GetGuid())};
                const Path oldAssetTempPath{MakeTempAssetPath(kAssetCategory, oldAssetInfo.GetGuid())};
                const Path oldAssetMetadataTempPath{MakeTempAssetMetadataPath(kAssetCategory, oldAssetInfo.GetGuid())};

                const bool bShouldTemporalizeAsset = !fs::exists(oldAssetTempPath) && fs::exists(oldAssetPath);
                const bool bShouldTemporalizeMeta = !fs::exists(oldAssetMetadataTempPath) && fs::exists(oldAssetMetadataPath);
                if (bShouldTemporalizeAsset && bShouldTemporalizeMeta)
                {
                    fs::copy(oldAssetPath, oldAssetTempPath);
                    fs::copy(oldAssetMetadataPath, oldAssetMetadataTempPath);
                }
                fs::remove(oldAssetPath);
                fs::remove(oldAssetMetadataPath);

                const Path newAssetPath{MakeAssetPath(kAssetCategory, assetInfo.GetGuid())};
                const Path newAssetMetadataPath{MakeAssetMetadataPath(kAssetCategory, assetInfo.GetGuid())};
                fs::rename(newAssetPath, oldAssetPath);
                fs::rename(newAssetMetadataPath, oldAssetMetadataPath);

                assetInfo.SetGuid(oldAssetInfo.GetGuid());
                bShouldReload = true;
            }

            assetMonitor->Create<T>(assetInfo, desc.LoadDescriptor);
            IG_LOG(AssetManagerLog, Info, "{}: \"{}\" imported as {} ({}).", kAssetCategory, resPath, virtualPath, assetInfo.GetGuid());

            if (bShouldReload)
            {
                Reload<T>(assetInfo.GetGuid());
            }

            if (!bShouldSuppressDirty)
            {
                bIsDirty = true;
            }

            return assetInfo.GetGuid();
        }

        template <typename T, typename AssetLoader>
        [[nodiscard]] Handle<T> LoadImpl(const Guid& guid, AssetLoader& loader, const bool bShouldSuppressDirty)
        {
            if (!assetMonitor->Contains(guid))
            {
                IG_LOG(AssetManagerLog, Error, "{} asset \"{}\" is invisible to asset manager.", AssetCategoryOf<T>, guid);
                return Handle<T>{};
            }

            AssetLock assetLock{GetAssetMutex(guid)};
            details::AssetCache<T>& assetCache{GetCache<T>()};
            if (!assetCache.IsCached(guid))
            {
                const typename T::Desc desc{assetMonitor->GetDesc<T>(guid)};
                IG_CHECK(desc.Info.GetGuid() == guid);
                auto result{loader.Load(desc)};
                if (!result.HasOwnership())
                {
                    IG_LOG(AssetManagerLog, Error, "Failed({}) to load {} asset {} ({}).", AssetCategoryOf<T>, result.GetStatus(),
                        desc.Info.GetVirtualPath(), guid);
                    return Handle<T>{};
                }

                assetCache.Cache(guid, result.Take());
                IG_LOG(AssetManagerLog, Info, "{} asset {} ({}) cached.", AssetCategoryOf<T>, desc.Info.GetVirtualPath(), guid);
            }

            IG_LOG(AssetManagerLog, Info, "Cache Hit! {} asset {} loaded.", AssetCategoryOf<T>, guid);
            if (!bShouldSuppressDirty)
            {
                bIsDirty = true;
            }
            return assetCache.Load(guid);
        }

        template <typename T, typename AssetLoader>
        bool ReloadImpl(const Guid& guid, const typename T::Desc& desc, AssetLoader& loader, const bool bShouldSuppressDirty)
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

            Handle<T> cachedAsset{assetCache.Load(guid, false)};
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

            if (!bShouldSuppressDirty)
            {
                bIsDirty = true;
            }

            return true;
        }

        void DeleteImpl(const EAssetCategory assetType, const Guid& guid, const bool bShouldSuppressDirty);

        [[nodiscard]] AssetMutex& GetAssetMutex(const Guid& guid);

        template <typename T, ResultStatus Status>
        void RegisterEngineInternalAsset(const std::string_view requiredVirtualPath, Result<T, Status> assetResult)
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
            std::optional<Guid> guidOpt{ImportImpl<T>(assetInfo.GetVirtualPath(), dummyResult, false)};
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
        Vector<Ptr<details::TypelessAssetCache>> assetCaches;

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

        Ptr<AudioClipImporter> audioImporter;
        Ptr<AudioClipLoader> audioLoader;

        std::atomic_bool bIsDirty{false};
        ModifiedEvent assetModifiedEvent;
    };
} // namespace ig
