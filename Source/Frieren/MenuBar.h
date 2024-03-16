#pragma once
#include <ImGui/ImGuiLayer.h>
#include <ImGui/CachedStringDebugger.h>
#include <ImGui/EntityList.h>
#include <Core/Igniter.h>

class MenuBar : public ig::ImGuiLayer
{
public:
    MenuBar(ig::CachedStringDebugger& cachedStringDebugger, ig::EntityList& entityList)
        : cachedStringDebugger(cachedStringDebugger),
          entityList(entityList),
          ig::ImGuiLayer(ig::String("MenuBar"))
    {
    }

    virtual void Render() override
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

                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

private:
    ig::CachedStringDebugger& cachedStringDebugger;
    ig::EntityList& entityList;
};