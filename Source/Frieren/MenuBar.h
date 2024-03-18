#pragma once
#include <Core/Igniter.h>
#include <ImGui/CachedStringDebugger.h>
#include <ImGui/EntityInsepctor.h>
#include <ImGui/EntityList.h>
#include <ImGui/ImGuiLayer.h>
#include <ImGui/StatisticsPanel.h>

namespace fe
{
    class MenuBar : public ig::ImGuiLayer
    {
    public:
        MenuBar(ig::StatisticsPanel& statisticsPanel, ig::CachedStringDebugger& cachedStringDebugger, ig::EntityList& entityList, ig::EntityInspector& entityInspector)
            : statisticsPanel(statisticsPanel),
              cachedStringDebugger(cachedStringDebugger),
              entityList(entityList),
              entityInspector(entityInspector)
        {
        }

        ~MenuBar() = default;

        void Render() override
        {
            if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("Exit (Alt+F4)"))
                    {
                        ig::Igniter::Exit();
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Debug"))
                {
                    if (ImGui::MenuItem("Statistics Panel"))
                    {
                        statisticsPanel.SetVisibility(true);
                    }

                    if (ImGui::MenuItem("Debug Cached String"))
                    {
                        cachedStringDebugger.SetVisibility(true);
                    }

                    if (ImGui::MenuItem("Entity List"))
                    {
                        entityList.SetVisibility(true);
                    }

                    if (ImGui::MenuItem("Entity Inspector"))
                    {
                        entityInspector.SetVisibility(true);
                    }

                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }
        }

    private:
        ig::StatisticsPanel& statisticsPanel;
        ig::CachedStringDebugger& cachedStringDebugger;
        ig::EntityList& entityList;
        ig::EntityInspector& entityInspector;
    };
}