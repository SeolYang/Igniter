#pragma once
#include <ImGui/ImGuiLayer.h>
#include <ImGui/CachedStringDebugger.h>
#include <Engine.h>

class MenuBar : public fe::ImGuiLayer
{
public:
    MenuBar(fe::CachedStringDebugger& cachedStringDebugger)
        : cachedStringDebugger(cachedStringDebugger),
          fe::ImGuiLayer(fe::String("MenuBar"))
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
                    fe::Engine::Exit();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Debug"))
            {
                if (ImGui::MenuItem("Debug Cached String"))
                {
                    cachedStringDebugger.SetVisibility(!cachedStringDebugger.IsVisible());
                }

                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

private:
    fe::CachedStringDebugger& cachedStringDebugger;
};