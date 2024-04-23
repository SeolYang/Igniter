#pragma once
#include <Frieren.h>
#include <Core/Engine.h>
#include <ImGui/ImGuiLayer.h>

namespace ig
{
    class ImGuiCanvas;
}

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

    class MainLayer final : public ig::ImGuiLayer
    {
    public:
        explicit MainLayer(ig::ImGuiCanvas& canvas);
        MainLayer(const MainLayer&) = delete;
        MainLayer(MainLayer&&) noexcept = delete;
        ~MainLayer() override = default;

        MainLayer& operator=(const MainLayer&) = delete;
        MainLayer& operator=(MainLayer&&) noexcept = delete;

        void Render() override;

    private:
        StatisticsPanel* statisticsPanel{nullptr};
        CachedStringDebugger* cachedStringDebugger{nullptr};
        EntityList* entityList{nullptr};
        EntityInspector* entityInspector{nullptr};
        AssetWatchPanel* assetWatchPanel{nullptr};
        TextureImportPanel* textureImportPanel{nullptr};
        StaticMeshImportPanel* staticMeshImportPanel{nullptr};
        AssetInspector* assetSnapshotPanel{nullptr};
    };
}    // namespace fe
