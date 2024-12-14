#pragma once
#include "Frieren/Frieren.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/ImGui/ImGuiCanvas.h"

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
    class TestApp;
    class EditorCanvas final : public ig::ImGuiCanvas
    {
    public:
        EditorCanvas(TestApp& testApp);
        ~EditorCanvas() override;

        void OnImGui() override;

    private:
        TestApp& app;
        ig::Ptr<StatisticsPanel> statisticsPanel{};
        ig::Ptr<CachedStringDebugger> cachedStringDebugger{};
        ig::Ptr<EntityList> entityList{};
        ig::Ptr<EntityInspector> entityInspector{};
        ig::Ptr<TextureImportPanel> textureImportPanel{};
        ig::Ptr<StaticMeshImportPanel> staticMeshImportPanel{};
        ig::Ptr<AssetInspector> assetInspector{};

        bool bStatisticsPanelOpend = false;
        bool bCachedStringDebuggerOpend = false;
        bool bEntityInspectorOpend = false;
        bool bEntityListOpend = false;
        bool bTextureImportPanelOpend = false;
        bool bStaticMeshImportPanelOpend = false;
        bool bAssetInspectorOpend = false;
    };
}    // namespace fe
