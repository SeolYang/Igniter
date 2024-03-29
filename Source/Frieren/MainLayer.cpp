#include <Frieren.h>
#include <MainLayer.h>
#include <Core/Engine.h>
#include <Asset/AssetManager.h>
#include <ImGui/ImGuiCanvas.h>
#include <ImGui/StatisticsPanel.h>
#include <ImGui/CachedStringDebugger.h>
#include <ImGui/EntityInsepctor.h>
#include <ImGui/EntityList.h>
#include <ImGui/AssetWatchPanel.h>
#include <ImGui/TextureImportPanel.h>
#include <ImGui/StaticMeshImportPanel.h>

namespace fe
{
    MainLayer::MainLayer(ig::ImGuiCanvas& canvas) : canvas(canvas),
                                                    statisticsPanel(canvas.AddLayer<ig::StatisticsPanel>()),
                                                    cachedStringDebugger(canvas.AddLayer<ig::CachedStringDebugger>()),
                                                    entityList(canvas.AddLayer<ig::EntityList>()),
                                                    entityInspector(canvas.AddLayer<ig::EntityInspector>(entityList)),
                                                    assetWatchPanel(canvas.AddLayer<ig::AssetWatchPanel>()),
                                                    textureImportPanel(canvas.AddLayer<ig::TextureImportPanel>()),
                                                    staticMeshImportPanel(canvas.AddLayer<ig::StaticMeshImportPanel>())
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
                if (ImGui::MenuItem("Save all Asset Metadata", "Ctrl+A+S"))
                {
                    ig::AssetManager& assetManager = ig::Igniter::GetAssetManager();
                    assetManager.SaveAllMetadataChanges();
                }

                if (ImGui::MenuItem("Import Texture", "Ctrl+T"))
                {
                    textureImportPanel.SetVisibility(true);
                }

                if (ImGui::MenuItem("Import Static Mesh"))
                {
                    staticMeshImportPanel.SetVisibility(true);
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

                if (ImGui::MenuItem("Asset Watch Panel"))
                {
                    assetWatchPanel.SetVisibility(true);
                }

                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }
} // namespace fe