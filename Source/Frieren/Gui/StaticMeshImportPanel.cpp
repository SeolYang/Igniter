#include "Frieren/Frieren.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Asset/AssetManager.h"
#include "Frieren/Gui/StaticMeshImportPanel.h"

namespace fe
{
    bool StaticMeshImportPanel::OnImGui()
    {
        if (ImGui::Button("Reselect"))
        {
            SelectFileToImport();
        }
        if (status != ig::EOpenFileDialogStatus::Success)
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
                ig::AssetManager& assetManager = ig::Engine::GetAssetManager();
                assetManager.Import(path, config);
                return false;
            }
        }

        return true;
    }

    void StaticMeshImportPanel::SelectFileToImport()
    {
        config = {};

        static const ig::Vector<ig::DialogFilter> Filters{
            ig::DialogFilter{.Name = "Model Resources"_fs, .FilterPattern = "*.fbx;*.obj;*.gltf"_fs},
        };

        ig::Result<ig::String, ig::EOpenFileDialogStatus> result = ig::OpenFileDialog::Show(nullptr, "Model resource to import"_fs, Filters);
        status = result.GetStatus();
        if (result.HasOwnership())
        {
            path = result.Take();
        }
    }
} // namespace fe
