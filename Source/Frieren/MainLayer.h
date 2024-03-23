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
    class FileNotificationPanel;
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
        ig::FileNotificationPanel& fileNotificationPanel;

    };
} // namespace fe