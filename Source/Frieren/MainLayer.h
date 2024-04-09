#pragma once
#include <Core/Engine.h>
#include <ImGui/ImGuiLayer.h>

namespace ig
{
    class ImGuiCanvas;
    class CachedStringDebugger;
    class EntityList;
    class EntityInspector;
    class StatisticsPanel;
    class AssetWatchPanel;
    class TextureImportPanel;
    class StaticMeshImportPanel;
    class AssetInspector;
} // namespace ig

namespace fe
{
    class MainLayer : public ig::ImGuiLayer
    {
    public:
        MainLayer(ig::ImGuiCanvas& canvas);
        ~MainLayer();

        void Render() override;

    private:
        ig::ImGuiCanvas& canvas;

        ig::StatisticsPanel& statisticsPanel;
        ig::CachedStringDebugger& cachedStringDebugger;
        ig::EntityList& entityList;
        ig::EntityInspector& entityInspector;
        ig::AssetWatchPanel& assetWatchPanel;
        ig::TextureImportPanel& textureImportPanel;
        ig::StaticMeshImportPanel& staticMeshImportPanel;
        ig::AssetInspector& assetSnapshotPanel;
    };
} // namespace fe