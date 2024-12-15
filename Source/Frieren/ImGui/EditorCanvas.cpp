#include "Frieren/Frieren.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Asset/AssetManager.h"
#include "Igniter/Gameplay/GameSystem.h"
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
        , statisticsPanel(ig::MakePtr<StatisticsPanel>())
        , cachedStringDebugger(ig::MakePtr<CachedStringDebugger>())
        , entityList(ig::MakePtr<EntityList>())
        , entityInspector(ig::MakePtr<EntityInspector>(*entityList))
        , textureImportPanel(ig::MakePtr<TextureImportPanel>())
        , staticMeshImportPanel(ig::MakePtr<StaticMeshImportPanel>())
        , assetInspector(ig::MakePtr<AssetInspector>())
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
                    ig::AssetManager& assetManager = ig::Engine::GetAssetManager();
                    assetManager.SaveAllChanges();
                }

                if (ImGui::MenuItem("Exit (Alt+F4)"))
                {
                    ig::Engine::Stop();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Editor"))
            {
                if (ImGui::MenuItem("Entities"))
                {
                    bEntityListOpend = !bEntityListOpend;
                    entityList->SetActiveWorld(app.GetActiveWorld());

                    bEntityInspectorOpend = !bEntityInspectorOpend;
                    entityInspector->SetActiveWorld(app.GetActiveWorld());
                }

                if (ImGui::MenuItem("Editor Camera", nullptr, nullptr, false))
                {
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

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Game Systems"))
            {
                for (auto&& [typeID, type] : entt::resolve())
                {
                    if (type.prop(ig::meta::GameSystemProperty))
                    {
                        if (ImGui::MenuItem(type.prop(ig::meta::TitleCaseNameProperty).value().cast<ig::String>().ToCString()))
                        {
                            app.SetGameSystem(
                                std::move(*type.func(ig::meta::GameSystemConstructFunc).invoke({}).try_cast<ig::Ptr<ig::GameSystem>>()));
                        }
                    }
                }

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        if (bStatisticsPanelOpend)
        {
            if (ImGui::Begin("Statistics", &bStatisticsPanelOpend))
            {
                statisticsPanel->OnImGui();
            }
            ImGui::End();
        }

        if (bCachedStringDebuggerOpend)
        {
            if (ImGui::Begin("Cached String Debugger", &bCachedStringDebuggerOpend))
            {
                cachedStringDebugger->OnImGui();
            }
            ImGui::End();
        }

        if (bEntityListOpend)
        {
            if (ImGui::Begin("Entity List", &bEntityListOpend))
            {
                entityList->OnImGui();
            }
            ImGui::End();
        }

        if (bEntityInspectorOpend)
        {
            if (ImGui::Begin("Entity Inspector", &bEntityInspectorOpend))
            {
                entityInspector->OnImGui();
            }
            ImGui::End();
        }

        if (bTextureImportPanelOpend)
        {
            if (ImGui::Begin("Texture Importer", &bTextureImportPanelOpend))
            {
                bTextureImportPanelOpend = textureImportPanel->OnImGui();
            }
            ImGui::End();
        }

        if (bStaticMeshImportPanelOpend)
        {
            if (ImGui::Begin("Static Mesh Importer", &bStaticMeshImportPanelOpend))
            {
                bStaticMeshImportPanelOpend = staticMeshImportPanel->OnImGui();
            }
            ImGui::End();
        }

        if (bAssetInspectorOpend)
        {
            if (ImGui::Begin("Asset Inspector", &bAssetInspectorOpend, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar))
            {
                assetInspector->OnImGui();
            }
            ImGui::End();
        }
    }
}    // namespace fe