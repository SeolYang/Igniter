#pragma once
#include <Frieren.h>
#include <Asset/AssetManager.h>
#include <ImGui/ImGuiLayer.h>

namespace fe
{
    class AssetInspector final : public ImGuiLayer
    {
    public:
        AssetInspector();
        AssetInspector(const AssetInspector&)     = delete;
        AssetInspector(AssetInspector&&) noexcept = delete;
        ~AssetInspector() override                = default;

        AssetInspector& operator=(const AssetInspector&)     = delete;
        AssetInspector& operator=(AssetInspector&&) noexcept = delete;

        void Render() override;

    private:
        void RenderMenuBar();
        void RenderFilterMenu();
        void RenderMainFrame();
        void RenderAssetStats();
        void RenderAssetTable(const EAssetCategory assetCategoryFilter, int& selectedIdx,
                              bool*                bSelectionDirtyFlagPtr = nullptr);
        void RenderInspector();
        void RenderEdit(const AssetInfo& assetInfo);
        void RenderMaterialEdit(const AssetInfo& assetInfo);
        void RenderStaticMeshEdit(const AssetInfo& assetInfo);
        void RenderPreview(const AssetInfo& assetInfo);
        void RenderTexturePreview(const AssetInfo& assetInfo);
        void RenderAssetInfo(const AssetInfo& assetInfo);
        void RenderSelector(const char* label, const Guid guid);
        int  RenderSelectorPopup(const EAssetCategory selectAssetType);

    private:
        RecursiveMutex                      mutex{};
        std::vector<AssetManager::Snapshot> snapshots{};
        bool                                bDirty{true};
        chrono::system_clock::time_point    lastUpdated{chrono::system_clock::now()};

        EAssetCategory mainTableAssetFilter{EAssetCategory::Unknown};
        int            mainTableSelectedIdx{-1};
        bool           bIsMainSelectionDirty = false;

        CachedAsset<Texture> previewTextures[NumFramesInFlight];
        bool                 bIsPreviewSrvUpdated[NumFramesInFlight]{false,};

        int  selectorTableSelectedIdx{-1};
        bool bOpenSelectorPopup = false;
    };
} // namespace ig
