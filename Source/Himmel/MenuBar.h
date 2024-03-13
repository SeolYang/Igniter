#pragma once
#include <ImGui/ImGuiLayer.h>
#include <Engine.h>

class MenuBar : public fe::ImGuiLayer
{
public:
	MenuBar()
		: fe::ImGuiLayer(fe::String("MenuBar"))
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
			ImGui::EndMainMenuBar();
		}
	}
};