#include "Frieren/Frieren.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Asset/AssetManager.h"
#include "Igniter/Gameplay/GameSystem.h"
#include "Frieren/Application/TestApp.h"
#include "Frieren/Gui/StatisticsPanel.h"
#include "Frieren/Gui/EntityInsepctor.h"
#include "Frieren/Gui/EntityList.h"
#include "Frieren/Gui/TextureImportPanel.h"
#include "Frieren/Gui/StaticMeshImportPanel.h"
#include "Frieren/Gui/AssetInspector.h"
#include "Frieren/Gui/EditorCanvas.h"

namespace fe
{
    EditorCanvas::EditorCanvas(TestApp& testApp)
        : app(testApp)
        , statisticsPanel(ig::MakePtr<StatisticsPanel>())
        , entityList(ig::MakePtr<EntityList>())
        , entityInspector(ig::MakePtr<EntityInspector>(*entityList))
        , textureImportPanel(ig::MakePtr<TextureImportPanel>())
        , staticMeshImportPanel(ig::MakePtr<StaticMeshImportPanel>())
        , assetInspector(ig::MakePtr<AssetInspector>())
    {}

    EditorCanvas::~EditorCanvas() {}

    void EditorCanvas::OnGui()
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
                    entityList->SetActiveWorld(&ig::Engine::GetWorld());

                    bEntityInspectorOpend = !bEntityInspectorOpend;
                    entityInspector->SetActiveWorld(&ig::Engine::GetWorld());
                }

                if (ImGui::MenuItem("Editor Camera", nullptr, nullptr, false))
                {}

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

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Game Systems"))
            {
                for (auto&& [typeID, type] : entt::resolve())
                {
                    if (type.prop(ig::meta::GameSystemProperty))
                    {
                        if (ImGui::MenuItem(type.prop(ig::meta::TitleCaseNameProperty).value().try_cast<std::string>()->c_str()))
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
                bTextureImportPanelOpend = bTextureImportPanelOpend && textureImportPanel->OnImGui();
            }
            ImGui::End();
        }

        if (bStaticMeshImportPanelOpend)
        {
            if (ImGui::Begin("Static Mesh Importer", &bStaticMeshImportPanelOpend))
            {
                bStaticMeshImportPanelOpend = bStaticMeshImportPanelOpend && staticMeshImportPanel->OnImGui();
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
} // namespace fe
