#pragma once
#include <Igniter.h>
#include <Asset/AssetManager.h>
#include <ImGui/ImGuiLayer.h>

namespace ig
{
    class AssetInspector : public ImGuiLayer
    {
    public:
        AssetInspector();
        ~AssetInspector() = default;

        void Render() override;

    private:
        void RenderMenuBar();
        void RenderFilterMenu();
        void RenderMainFrame();
        void RenderAssetStats();
        void RenderAssetTable(const EAssetType assetTypeFilter, int& selectedIdx, bool* bSelectionDirtyFlagPtr = nullptr);
        void RenderInspector();
        void RenderEdit(const AssetInfo& assetInfo);
        void RenderMaterialEdit(const AssetInfo& assetInfo);
        void RenderStaticMeshEdit(const AssetInfo& assetInfo);
        void RenderPreview(const AssetInfo& assetInfo);
        void RenderTexturePreview(const AssetInfo& assetInfo);
        void RenderAssetInfo(const AssetInfo& assetInfo);
        void RenderSelector(const char* label, const Guid guid);
        int RenderSelectorPopup(const EAssetType selectAssetType);

    private:
        RecursiveMutex mutex{};
        std::vector<AssetManager::Snapshot> snapshots{};
        bool bDirty{ true };
        chrono::system_clock::time_point lastUpdated{ chrono::system_clock::now() };

        EAssetType mainTableAssetFilter{EAssetType::Unknown};
        int mainTableSelectedIdx{ -1 };
        bool bIsMainSelectionDirty = false;
        CachedAsset<Texture> previewTextures[NumFramesInFlight];
        bool bIsPreviewSrvUpdated[NumFramesInFlight]{ false, };

        int selectorTableSelectedIdx{ -1 };
        bool bOpenSelectorPopup = false;
    };
} // namespace ig