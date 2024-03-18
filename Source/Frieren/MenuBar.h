#pragma once
#include <Core/Igniter.h>
#include <ImGui/CachedStringDebugger.h>
#include <ImGui/EntityInsepctor.h>
#include <ImGui/EntityList.h>
#include <ImGui/ImGuiLayer.h>

class MenuBar : public ig::ImGuiLayer
{
public:
    MenuBar(ig::CachedStringDebugger& cachedStringDebugger, ig::EntityList& entityList, ig::EntityInspector& entityInspector)
        : cachedStringDebugger(cachedStringDebugger),
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
    ig::CachedStringDebugger& cachedStringDebugger;
    ig::EntityList& entityList;
    ig::EntityInspector& entityInspector;
};