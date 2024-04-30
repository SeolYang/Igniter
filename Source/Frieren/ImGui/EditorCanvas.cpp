#include <Frieren.h>
#include <Core/Engine.h>
#include <Asset/AssetManager.h>
#include <ImGui/ImGuiCanvas.h>
#include <ImGui/StatisticsPanel.h>
#include <ImGui/CachedStringDebugger.h>
#include <ImGui/EntityInsepctor.h>
#include <ImGui/EntityList.h>
#include <ImGui/TextureImportPanel.h>
#include <ImGui/StaticMeshImportPanel.h>
#include <ImGui/AssetInspector.h>
#include <ImGui/EditorCanvas.h>

namespace fe
{
    EditorCanvas::EditorCanvas()
        : statisticsPanel(MakePtr<StatisticsPanel>())
        , cachedStringDebugger(MakePtr<CachedStringDebugger>())
        , entityList(MakePtr<EntityList>())
        , entityInspector(MakePtr<EntityInspector>(*entityList))
        , textureImportPanel(MakePtr<TextureImportPanel>())
        , staticMeshImportPanel(MakePtr<StaticMeshImportPanel>())
        , assetInspector(MakePtr<AssetInspector>())
    {
    }

    EditorCanvas::~EditorCanvas() {}

    void EditorCanvas::OnImGui()
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
                    ig::Igniter::Stop();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Assets"))
            {
                if (ImGui::MenuItem("Asset Inspector"))
                {
                    bAssetInspectorOpend = !bAssetInspectorOpend;
                }

                if (ImGui::MenuItem("Import Texture"))
                {
                    bTextureImportPanelOpend = !bTextureImportPanelOpend;
                    if (bTextureImportPanelOpend)
                    {
                        textureImportPanel->SelectFileToImport();
                    }
                }

                if (ImGui::MenuItem("Import Static Mesh"))
                {
                    bStaticMeshImportPanelOpend = !bStaticMeshImportPanelOpend;
                    if (bStaticMeshImportPanelOpend)
                    {
                        staticMeshImportPanel->SelectFileToImport();
                    }
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Debug"))
            {
                if (ImGui::MenuItem("Statistics Panel"))
                {
                    bStatisticsPanelOpend = !bStatisticsPanelOpend;
                }

                if (ImGui::MenuItem("Debug Cached String"))
                {
                    bCachedStringDebuggerOpend = !bCachedStringDebuggerOpend;
                }

                if (ImGui::MenuItem("Entity Inspector"))
                {
                    bEntityInspectorOpend = !bEntityInspectorOpend;
                }

                if (ImGui::MenuItem("Entity Outliner"))
                {
                    bEntityListOpend = !bEntityListOpend;
                }

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        if (bStatisticsPanelOpend && ImGui::Begin("Statistics", &bStatisticsPanelOpend))
        {
            statisticsPanel->OnImGui();
            ImGui::End();
        }

        if (bCachedStringDebuggerOpend && ImGui::Begin("Cached String Debugger", &bCachedStringDebuggerOpend))
        {
            cachedStringDebugger->OnImGui();
            ImGui::End();
        }

        if (bEntityListOpend && ImGui::Begin("Entity Outliner", &bEntityListOpend))
        {
            entityList->OnImGui();
            ImGui::End();
        }

        if (bEntityInspectorOpend && ImGui::Begin("Entity Inspector", &bEntityInspectorOpend))
        {
            entityInspector->OnImGui();
            ImGui::End();
        }

        if (bTextureImportPanelOpend && ImGui::Begin("Texture Importer", &bTextureImportPanelOpend))
        {
            bTextureImportPanelOpend = textureImportPanel->OnImGui();
            ImGui::End();
        }

        if (bStaticMeshImportPanelOpend && ImGui::Begin("Static Mesh Importer", &bStaticMeshImportPanelOpend))
        {
            bStaticMeshImportPanelOpend = staticMeshImportPanel->OnImGui();
            ImGui::End();
        }

        if (bAssetInspectorOpend && ImGui::Begin("Asset Inspector", &bAssetInspectorOpend, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar))
        {
            assetInspector->OnImGui();
            ImGui::End();
        }
    }
}    // namespace fe