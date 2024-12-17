#pragma once
#include "Frieren/Frieren.h"
#include "Igniter/Asset/AssetManager.h"

namespace fe
{
    class AssetInspector final
    {
    public:
        AssetInspector();
        AssetInspector(const AssetInspector&) = delete;
        AssetInspector(AssetInspector&&) noexcept = delete;
        ~AssetInspector();

        AssetInspector& operator=(const AssetInspector&) = delete;
        AssetInspector& operator=(AssetInspector&&) noexcept = delete;

        void OnImGui();

    private:
        void RenderMenuBar();
        void RenderFilterMenu();
        void RenderMainFrame();
        void RenderAssetStats();
        void RenderAssetTable(const ig::EAssetCategory assetCategoryFilter, int& selectedIdx, bool* bSelectionDirtyFlagPtr = nullptr);
        void RenderInspector();
        void RenderEdit(const ig::AssetInfo& assetInfo);
        void RenderMaterialEdit(const ig::AssetInfo& assetInfo);
        void RenderStaticMeshEdit(const ig::AssetInfo& assetInfo);
        void RenderPreview(const ig::AssetInfo& assetInfo);
        void RenderTexturePreview(const ig::AssetInfo& assetInfo);
        void RenderAssetInfo(const ig::AssetInfo& assetInfo);
        void RenderSelector(const char* label, const ig::Guid guid);
        int RenderSelectorPopup(const ig::EAssetCategory selectAssetType);

    private:
        ig::RecursiveMutex mutex{};
        std::vector<ig::AssetManager::Snapshot> snapshots{};
        bool bDirty{true};
        ig::chrono::system_clock::time_point lastUpdated{ig::chrono::system_clock::now()};

        ig::EAssetCategory mainTableAssetFilter{ig::EAssetCategory::Unknown};
        int mainTableSelectedIdx{-1};
        bool bIsMainSelectionDirty = false;

        ig::ManagedAsset<ig::Texture> previewTexture{};

        int selectorTableSelectedIdx{-1};
        bool bOpenSelectorPopup = false;
    };
}    // namespace fe
