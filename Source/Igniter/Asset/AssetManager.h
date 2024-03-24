#pragma once
#include <Asset/Common.h>
#include <Asset/Texture.h>

namespace ig
{
    template <typename AssetType>
    using AssetRefHandle = RefHandle<AssetType, class AssetManager*>;
    class AsyncFileTracker;
    struct FileNotification;
    class AssetManager final
    {
        friend class RefHandle<Texture, AssetManager*>;
        friend class Handle<Texture, AssetManager*>;
        using VirtualPathGuidTable = robin_hood::unordered_map<String, xg::Guid>;

    public:
        AssetManager();
        AssetManager(const AssetManager&) = delete;
        AssetManager(AssetManager&&) noexcept = delete;
        ~AssetManager();

        AssetManager& operator=(const AssetManager&) = delete;
        AssetManager& operator=(AssetManager&&) noexcept = delete;

        bool ImportTexture(const String resPath, std::optional<TextureImportConfig> config = std::nullopt, std::optional<String> virtualPath = std::nullopt, const bool bIsPersistent = false);
        AssetRefHandle<Texture> LoadTexture(const xg::Guid& guid);
        AssetRefHandle<Texture> LoadTexture(const String virtualPath);

        void Unload(const xg::Guid& guid);
        void Unload(const EAssetType assetType, const String virtualPath);

        void ProcessFileNotifications();

    private:
        template <typename AssetType>
        void operator()(const RefHandle<AssetType, AssetManager*>&, AssetType*)
        {
            IG_UNIMPLEMENTED();
        }

        VirtualPathGuidTable& GetVirtualPathGuidTable(const EAssetType assetType);

        /* "/Assets" 폴더에서 리소스의 메타데이터를 읽어와서, 초기 virtualPahtTable과 guidAssetInfoTable 설정 */
        void ParseInitialAssetDirectory();

        void ProcessFileNotification(const FileNotification& notification);

    private:
        std::unique_ptr<AsyncFileTracker> assetDirTracker;

        SharedMutex bufferMutex;
        std::vector<FileNotification> notificationBuffer;

        /* #sy_log EAssetType의 종류가 상대적으로 작기 때문에 vector 에 저장하고 linear search 하더라도 충분 */
        std::vector<std::pair<EAssetType, VirtualPathGuidTable>> virtualPathGuidTables;
        robin_hood::unordered_map<xg::Guid, AssetInfo> guidAssetInfoTable;

    };

} // namespace ig