#include "Frieren/Frieren.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Asset/AssetManager.h"
#include "Igniter/Gameplay/GameMode.h"
#include "Igniter/ImGui/ImGuiCanvas.h"
#include "Frieren/Application/TestApp.h"
#include "Frieren/ImGui/StatisticsPanel.h"
#include "Frieren/ImGui/CachedStringDebugger.h"
#include "Frieren/ImGui/EntityInsepctor.h"
#include "Frieren/ImGui/EntityList.h"
#include "Frieren/ImGui/TextureImportPanel.h"
#include "Frieren/ImGui/StaticMeshImportPanel.h"
#include "Frieren/ImGui/AssetInspector.h"
#include "Frieren/ImGui/EditorCanvas.h"

namespace fe
{
    EditorCanvas::EditorCanvas(TestApp& testApp)
        : app(testApp)
        , statisticsPanel(MakePtr<StatisticsPanel>())
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
                    entityInspector->SetActiveWorld(app.GetActiveWorld());
                    entityList->SetActiveWorld(app.GetActiveWorld());
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Game Mode"))
            {
                for (auto&& [typeID, type] : entt::resolve())
                {
                    if (type.prop(ig::meta::GameModeProperty))
                    {
                        if (ImGui::MenuItem(type.prop(ig::meta::TitleCaseNameProperty).value().cast<ig::String>().ToCString()))
                        {
                            app.SetGameMode(std::move(*type.func(ig::meta::CreateGameModeFunc).invoke({}).try_cast<Ptr<GameMode>>()));
                        }
                    }
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

        if (bEntityInspectorOpend && ImGui::Begin("Entity Outliner", &bEntityInspectorOpend))
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