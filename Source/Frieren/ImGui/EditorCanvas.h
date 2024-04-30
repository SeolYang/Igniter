#pragma once
#include <Frieren.h>
#include <Core/Engine.h>
#include <ImGui/ImGuiCanvas.h>

namespace fe
{
    class CachedStringDebugger;
    class EntityList;
    class EntityInspector;
    class StatisticsPanel;
    class AssetWatchPanel;
    class TextureImportPanel;
    class StaticMeshImportPanel;
    class AssetInspector;
    class EditorCanvas final : public ig::ImGuiCanvas
    {
    public:
        EditorCanvas();
        ~EditorCanvas() override;

        void OnImGui() override;

    private:
        Ptr<StatisticsPanel> statisticsPanel{};
        Ptr<CachedStringDebugger> cachedStringDebugger{};
        Ptr<EntityList> entityList{};
        Ptr<EntityInspector> entityInspector{};
        Ptr<TextureImportPanel> textureImportPanel{};
        Ptr<StaticMeshImportPanel> staticMeshImportPanel{};
        Ptr<AssetInspector> assetInspector{};

        bool bStatisticsPanelOpend = false;
        bool bCachedStringDebuggerOpend = false;
        bool bEntityListOpend = false;
        bool bEntityInspectorOpend = false;
        bool bTextureImportPanelOpend = false;
        bool bStaticMeshImportPanelOpend = false;
        bool bAssetInspectorOpend = false;
    };
}    // namespace fe
