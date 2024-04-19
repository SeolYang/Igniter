#include <Frieren.h>
#include <Core/Engine.h>
#include <Asset/AssetManager.h>
#include <ImGui/StaticMeshImportPanel.h>

namespace fe
{
    void StaticMeshImportPanel::Render()
    {
        if (ImGui::Begin("Static Mesh Import", &bIsVisible))
        {
            if (ImGui::Button("Reselect"))
            {
                SelectFileToImport();
            }
            if (status != EOpenFileDialogStatus::Success)
            {
                ImGui::Text("Failed to select model resource file. (Status: %s)", magic_enum::enum_name(status).data());
            }
            else
            {
                ImGui::SameLine();
                ImGui::Text("%s", path.ToStringView().data());

                ImGui::Checkbox("Make Left Handed", &config.bMakeLeftHanded);
                ImGui::Checkbox("Flip UV Coordinates", &config.bFlipUVs);
                ImGui::Checkbox("Flip Winding Order", &config.bFlipWindingOrder);
                ImGui::Checkbox("Generate Normals", &config.bGenerateNormals);
                ImGui::Checkbox("Split Large Meshes", &config.bSplitLargeMeshes);
                ImGui::Checkbox("Pre-Transform Vertices", &config.bPreTransformVertices);
                ImGui::Checkbox("Improve Cache Locality", &config.bImproveCacheLocality);
                ImGui::Checkbox("Generate UV Coordinates", &config.bGenerateUVCoords);
                ImGui::Checkbox("Generate Bounding Boxes", &config.bGenerateBoundingBoxes);
                ImGui::Checkbox("Import Materials", &config.bImportMaterials);

                if (ImGui::Button("Import"))
                {
                    AssetManager& assetManager = Igniter::GetAssetManager();
                    assetManager.Import(path, config);
                    SetVisibility(false);
                }
            }

            ImGui::End();
        }
    }

    void StaticMeshImportPanel::OnVisible()
    {
        SelectFileToImport();
    }

    void StaticMeshImportPanel::SelectFileToImport()
    {
        config = {};

        static const std::vector<DialogFilter> Filters{
            DialogFilter{.Name = "Model Resources"_fs, .FilterPattern = "*.fbx;*.obj;*.gltf"_fs},
        };

        Result<String, EOpenFileDialogStatus> result = OpenFileDialog::Show(
            nullptr, "Model resource to import"_fs, Filters);
        status = result.GetStatus();
        if (result.HasOwnership())
        {
            path = result.Take();
        }
    }
} // namespace ig
