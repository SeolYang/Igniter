#include <Igniter.h>
#include <Core/Log.h>
#include <Core/Serializable.h>
#include <Render/GpuViewManager.h>
#include <Asset/AssetManager.h>
#include <Asset/AssetMonitor.h>
#include <Asset/Texture.h>
#include <Asset/Utils.h>

IG_DEFINE_LOG_CATEGORY(AssetManager);

namespace ig
{
    AssetManager::AssetManager() : assetMonitor(std::make_unique<AssetMonitor>()),
                                   textureImporter(std::make_unique<TextureImporter>())
    {
    }

    AssetManager::~AssetManager()
    {
    }

    void AssetManager::Update()
    {
        IG_CHECK(assetMonitor != nullptr);
        assetMonitor->ProcessBufferedNotifications();

        if (assetMonitor->HasExpiredAssets())
        {
            std::vector<AssetInfo> expiredAssetInfos = assetMonitor->FlushExpiredAssetInfos();
            for (const auto& assetInfo : expiredAssetInfos)
            {
                const fs::path assetPath{ MakeAssetPath(assetInfo.Type, assetInfo.Guid) };
                const fs::path assetMetadataPath{ MakeAssetMetadataPath(assetInfo.Type, assetInfo.Guid) };

                if (fs::exists(assetPath))
                {
                    IG_VERIFY(fs::remove(assetPath));
                }

                if (fs::exists(assetMetadataPath))
                {
                    IG_VERIFY(fs::remove(assetMetadataPath));
                }

                // #sy_wip Force Unload Expired Assets!
                IG_LOG(AssetManager, Warning, "Asset {} expired.", assetPath.string());
            }
        }
    }

    Result<xg::Guid, EAssetImportResult> AssetManager::ImportTexture(const String resPath, const TextureImportConfig& config)
    {
        IG_CHECK(textureImporter != nullptr);
        auto importedGuid = textureImporter->Import(resPath, config);
        if (!importedGuid)
        {
            return MakeFail<xg::Guid, EAssetImportResult::ImporterFailure>();
        }

        // 성공 한 직후, 바로 Load 가능해야 하기 때문에 강제로 처리해주어야 함.
        /* 이 지점에서, 확실하게 받아올 수 있는 방법이 필요 */
        Update();
        IG_CHECK(assetMonitor->Contains(*importedGuid));
        return MakeSuccess<xg::Guid, EAssetImportResult>(*importedGuid);
    }
} // namespace ig