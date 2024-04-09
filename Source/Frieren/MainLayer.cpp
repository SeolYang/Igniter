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
#include <ImGui/AssetInspector.h>

namespace fe
{
    MainLayer::MainLayer(ig::ImGuiCanvas& canvas)
        : canvas(canvas),
          statisticsPanel(canvas.AddLayer<ig::StatisticsPanel>()),
          cachedStringDebugger(canvas.AddLayer<ig::CachedStringDebugger>()),
          entityList(canvas.AddLayer<ig::EntityList>()),
          entityInspector(canvas.AddLayer<ig::EntityInspector>(entityList)),
          assetWatchPanel(canvas.AddLayer<ig::AssetWatchPanel>()),
          textureImportPanel(canvas.AddLayer<ig::TextureImportPanel>()),
          staticMeshImportPanel(canvas.AddLayer<ig::StaticMeshImportPanel>()),
          assetSnapshotPanel(canvas.AddLayer<ig::AssetInspector>())
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
                if (ImGui::MenuItem("Save All Asset Changes", "Ctrl+A+S"))
                {
                    ig::AssetManager& assetManager = ig::Igniter::GetAssetManager();
                    assetManager.SaveAllChanges();
                }

                if (ImGui::MenuItem("Exit (Alt+F4)"))
                {
                    ig::Igniter::Exit();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Assets"))
            {
                if (ImGui::MenuItem("Asset Inspector"))
                {
                    assetSnapshotPanel.SetVisibility(true);
                }

                if (ImGui::MenuItem("Import Texture"))
                {
                    textureImportPanel.SetVisibility(true);
                }

                if (ImGui::MenuItem("Import Static Mesh"))
                {
                    staticMeshImportPanel.SetVisibility(true);
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

                if (ImGui::MenuItem("Entity Inspector"))
                {
                    entityInspector.SetVisibility(true);
                    entityList.SetVisibility(true);
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