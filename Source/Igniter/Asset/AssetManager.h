#pragma once
#include <Core/Result.h>
#include <Core/Log.h>
#include <Core/Event.h>
#include <Asset/Common.h>
#include <Asset/AssetMonitor.h>
#include <Asset/AssetCache.h>
#include <Asset/Texture.h>
#include <Asset/StaticMesh.h>
#include <Asset/Material.h>

IG_DEFINE_LOG_CATEGORY(AssetManager);

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
            bool IsCached() const
            {
                return RefCount > 0 ||
                       Info.GetScope() == EAssetScope::Static ||
                       Info.GetScope() == EAssetScope::Engine;
            }

        public:
            AssetInfo Info{};
            uint32_t RefCount{};
        };

    public:
        AssetManager(HandleManager& handleManager, RenderDevice& renderDevice, GpuUploader& gpuUploader, GpuViewManager& gpuViewManager);
        AssetManager(const AssetManager&) = delete;
        AssetManager(AssetManager&&) noexcept = delete;
        ~AssetManager();

        AssetManager& operator=(const AssetManager&) = delete;
        AssetManager& operator=(AssetManager&&) noexcept = delete;

        Guid Import(const String resPath, const TextureImportDesc& desc);
        [[nodiscard]] CachedAsset<Texture> LoadTexture(const Guid guid);
        [[nodiscard]] CachedAsset<Texture> LoadTexture(const String virtualPath);

        std::vector<Guid> Import(const String resPath, const StaticMeshImportDesc& desc);
        [[nodiscard]] CachedAsset<StaticMesh> LoadStaticMesh(const Guid guid);
        [[nodiscard]] CachedAsset<StaticMesh> LoadStaticMesh(const String virtualPath);

        Guid Import(const String virtualPath, const MaterialCreateDesc& createDesc);
        [[nodiscard]] CachedAsset<Material> LoadMaterial(const Guid guid);
        [[nodiscard]] CachedAsset<Material> LoadMaterial(const String virtualPath);

        template <Asset T>
        bool Reload(const Guid guid)
        {
            if (!assetMonitor->Contains(guid))
            {
                IG_LOG(AssetManager, Error, "{} asset \"{}\" is invisible to asset manager.", AssetTypeOf_v<T>, guid);
                return false;
            }

            const typename T::Desc desc{ assetMonitor->GetDesc<T>(guid) };
            if constexpr (AssetTypeOf_v<T> == EAssetType::Texture)
            {
                return ReloadImpl<Texture>(guid, desc, *textureLoader);
            }
            else if (AssetTypeOf_v<T> == EAssetType::StaticMesh)
            {
                return ReloadImpl<StaticMesh>(guid, desc, *staticMeshLoader);
            }
            else if (AssetTypeOf_v<T> == EAssetType::Material)
            {
                return ReloadImpl<Material>(guid, desc, *materialLoader);
            }
            else
            {
                IG_CHECK_NO_ENTRY();
                IG_LOG(AssetManager, Error, "Reload Unsupported Asset Type {}.", AssetTypeOf_v<T>);
            }

            return false;
        }

        template <Asset T>
        bool Reimport(const Guid guid, const String resPath, const typename T::ImportDesc& importDesc)
        {
            /* #sy_todo Reimport 구현 할 것 */
            IG_UNIMPLEMENTED();
            return false;
        }

        void Delete(const Guid guid);
        void Delete(const EAssetType assetType, const String virtualPath);

        void SaveAllChanges();

        [[nodiscard]] AssetInfo GetAssetInfo(const Guid guid) const;
        [[nodiscard]] ModifiedEvent& GetModifiedEvent() { return assetModifiedEvent; }

        [[nodiscard]] std::vector<Snapshot> TakeSnapshots() const;

    private:
        template <Asset T>
        details::AssetCache<T>& GetCache()
        {
            return static_cast<details::AssetCache<T>&>(GetTypelessCache(AssetTypeOf_v<T>));
        }

        details::TypelessAssetCache& GetTypelessCache(const EAssetType assetType);

        void InitAssetCaches(HandleManager& handleManager);

        void InitEngineInternalAssets();

        template <Asset T, ResultStatus ImportStatus>
        std::optional<Guid> ImportImpl(String resPath, Result<typename T::Desc, ImportStatus>& result)
        {
            constexpr auto AssetType{ AssetTypeOf_v<T> };
            if (!result.HasOwnership())
            {
                IG_LOG(AssetManager, Error, "{}: Failed({}) to import \"{}\".",
                       AssetType,
                       result.GetStatus(),
                       resPath);

                return std::nullopt;
            }

            const T::Desc desc{ result.Take() };
            const AssetInfo& assetInfo{ desc.Info };
            IG_CHECK(assetInfo.IsValid());
            IG_CHECK(assetInfo.GetType() == AssetType);

            const String virtualPath{ assetInfo.GetVirtualPath() };
            IG_CHECK(IsValidVirtualPath(virtualPath));

            if (assetMonitor->Contains(AssetType, virtualPath))
            {
                const AssetInfo currentAssetInfo{ assetMonitor->GetAssetInfo(AssetType, virtualPath) };
                IG_CHECK(currentAssetInfo.IsValid());
                if (currentAssetInfo.GetScope() == EAssetScope::Engine)
                {
                    IG_LOG(AssetManager, Error, "{}: Failed to import \"{}\". Given virtual path {} was reserved by engine.",
                           AssetType,
                           resPath,
                           virtualPath);

                    return std::nullopt;
                }

                Delete(AssetType, virtualPath);
            }
            assetMonitor->Create<T>(assetInfo, desc.LoadDescriptor);

            IG_LOG(AssetManager, Info, "{}: \"{}\" imported as {} ({}).",
                   AssetType,
                   resPath,
                   virtualPath,
                   assetInfo.GetGuid());
            return assetInfo.GetGuid();
        }
 
        template <Asset T, typename AssetLoader>
        [[nodiscard]] CachedAsset<T> LoadImpl(const Guid guid, AssetLoader& loader)
        {
            if (!assetMonitor->Contains(guid))
            {
                IG_LOG(AssetManager, Error, "{} asset \"{}\" is invisible to asset manager.", AssetTypeOf_v<T>, guid);
                return CachedAsset<T>{};
            }

            AssetLock assetLock{ RequestAssetMutex(guid) };
            details::AssetCache<T>& assetCache{ GetCache<T>() };
            if (!assetCache.IsCached(guid))
            {
                const T::Desc desc{ assetMonitor->GetDesc<T>(guid) };
                IG_CHECK(desc.Info.GetGuid() == guid);
                auto result{ loader.Load(desc) };
                if (result.HasOwnership())
                {
                    IG_LOG(AssetManager, Info, "{} asset {} ({}) cached.",
                           AssetTypeOf_v<T>,
                           desc.Info.GetVirtualPath(), guid);
                    assetModifiedEvent.Notify(*this);
                    return assetCache.Cache(guid, result.Take());
                }

                IG_LOG(AssetManager, Error, "Failed({}) to load {} asset {} ({}).",
                       AssetTypeOf_v<T>, result.GetStatus(),
                       desc.Info.GetVirtualPath(), guid);
                return CachedAsset<T>{};
            }

            IG_LOG(AssetManager, Info, "Cache Hit! {} asset {} loaded.", AssetTypeOf_v<T>, guid);
            assetModifiedEvent.Notify(*this);
            return assetCache.Load(guid);
        }

        template <Asset T, typename AssetLoader>
        bool ReloadImpl(const Guid guid, const typename T::Desc& desc, AssetLoader& loader)
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

            auto result{ loader.Load(desc) };
            if (!result.HasOwnership())
            {
                IG_LOG(AssetManager, Error, "{} asset \"{}\" failed to reload.", AssetTypeOf_v<T>, guid);
                return false;
            }

            {
                AssetLock assetLock{ RequestAssetMutex(guid) };
                details::AssetCache<T>& assetCache{ GetCache<T>() };
                CachedAsset<T> cachedAsset{ assetCache.Load(guid) };
                if (cachedAsset)
                {
                    *cachedAsset = result.Take();
                }
                else
                {
                    cachedAsset = assetCache.Cache(guid, result.Take());
                }
                IG_CHECK(cachedAsset);
            }

            assetModifiedEvent.Notify(*this);
            return true;
        }

        void DeleteImpl(const EAssetType assetType, const Guid guid);

        [[nodiscard]] AssetMutex& RequestAssetMutex(const Guid guid);

        template <Asset T, ResultStatus Status>
        void RegisterEngineInternal(const String requiredVirtualPath, Result<T, Status> assetResult)
        {
            if (!assetResult.HasOwnership())
            {
                IG_LOG(AssetManager, Error, "{}: Failed({}) to create engine default asset {}.",
                       AssetTypeOf_v<T>, assetResult.GetStatus(), requiredVirtualPath);
                return;
            }

            T asset{ assetResult.Take() };
            const typename T::Desc& desc{ asset.GetSnapshot() };
            const AssetInfo& assetInfo{ desc.Info };
            IG_CHECK(assetInfo.IsValid());
            IG_CHECK(assetInfo.GetType() == AssetTypeOf_v<T>);
            IG_CHECK(assetInfo.GetScope() == EAssetScope::Engine);

            /* #sy_note GUID를 고정으로 할지 말지.. 일단 Virtual Path는 고정적이고, Import 되고난 이후엔 Virtual Path로 접근 가능하니 유동적으로 */
            Result<typename T::Desc, details::EDefaultDummyStatus> dummyResult{ MakeSuccess<typename T::Desc, details::EDefaultDummyStatus>(desc) };
            IG_CHECK(dummyResult.HasOwnership());
            std::optional<Guid> guidOpt{ ImportImpl<T>(assetInfo.GetVirtualPath(), dummyResult) };
            IG_CHECK(guidOpt);

            const Guid guid{ *guidOpt };
            IG_CHECK(guid.isValid());
            AssetLock assetLock{ RequestAssetMutex(guid) };
            details::AssetCache<T>& assetCache{ GetCache<T>() };
            IG_CHECK(!assetCache.IsCached(guid));
            [[maybe_unused]] CachedAsset<T> cachedAsset{ assetCache.Cache(guid, std::move(asset)) };
            IG_CHECK(cachedAsset);
        }

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

        ModifiedEvent assetModifiedEvent;
    };
} // namespace ig