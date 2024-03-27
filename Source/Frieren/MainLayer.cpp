#include <Frieren.h>
#include <MainLayer.h>
#include <ImGui/ImGuiCanvas.h>
#include <ImGui/StatisticsPanel.h>
#include <ImGui/CachedStringDebugger.h>
#include <ImGui/EntityInsepctor.h>
#include <ImGui/EntityList.h>
#include <ImGui/FileNotificationPanel.h>
#include <ImGui/TextureImportPanel.h>

namespace fe
{
    MainLayer::MainLayer(ig::ImGuiCanvas& canvas) : canvas(canvas),
                                                    statisticsPanel(canvas.AddLayer<ig::StatisticsPanel>()),
                                                    cachedStringDebugger(canvas.AddLayer<ig::CachedStringDebugger>()),
                                                    entityList(canvas.AddLayer<ig::EntityList>()),
                                                    entityInspector(canvas.AddLayer<ig::EntityInspector>(entityList)),
                                                    fileNotificationPanel(canvas.AddLayer<ig::FileNotificationPanel>()),
                                                    textureImportPanel(canvas.AddLayer<ig::TextureImportPanel>())
    {
    }

    MainLayer::~MainLayer()
    {
    }

    void MainLayer::Render()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Import Texture", "Ctrl+T"))
                {
                    textureImportPanel.SetVisibility(true);
                }

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

                if (ImGui::MenuItem("File Notification Panel"))
                {
                    fileNotificationPanel.SetVisibility(true);
                }

                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }
} // namespace fe