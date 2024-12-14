#include "Frieren/Frieren.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Asset/AssetManager.h"
#include "Igniter/ImGui/ImGuiExtensions.h"
#include "Frieren/ImGui/TextureImportPanel.h"

namespace fe
{
    bool TextureImportPanel::OnImGui()
    {
        if (ImGui::Button("Reselect"))
        {
            SelectFileToImport();
        }

        if (status != ig::EOpenFileDialogStatus::Success)
        {
            ImGui::Text("Failed to select texture resource file. (Status: %s)", magic_enum::enum_name(status).data());
        }
        else
        {
            ImGui::SameLine();
            ImGui::Text("%s", path.ToStringView().data());

            if (ig::ImGuiX::BeginEnumCombo<ig::ETextureCompressionMode>("Compression Mode", selectedCompModeIdx))
            {
                ig::ImGuiX::EndEnumCombo();
            }

            ImGui::Checkbox("Generate Mips", &config.bGenerateMips);

            if (ig::ImGuiX::BeginEnumCombo<D3D12_FILTER>("Filter", selectedFilterIdx))
            {
                ig::ImGuiX::EndEnumCombo();
            }

            if (ig::ImGuiX::BeginEnumCombo<D3D12_TEXTURE_ADDRESS_MODE>("Address Mode(U)", selectedAddressModeU))
            {
                ig::ImGuiX::EndEnumCombo();
            }

            if (ig::ImGuiX::BeginEnumCombo<D3D12_TEXTURE_ADDRESS_MODE>("Address Mode(V)", selectedAddressModeV))
            {
                ig::ImGuiX::EndEnumCombo();
            }

            if (ig::ImGuiX::BeginEnumCombo<D3D12_TEXTURE_ADDRESS_MODE>("Address Mode(W)", selectedAddressModeW))
            {
                ig::ImGuiX::EndEnumCombo();
            }

            if (ImGui::Button("Import"))
            {
                constexpr auto CompressionModes = magic_enum::enum_values<ig::ETextureCompressionMode>();
                constexpr auto Filters = magic_enum::enum_values<D3D12_FILTER>();
                constexpr auto AddressModes = magic_enum::enum_values<D3D12_TEXTURE_ADDRESS_MODE>();

                config.CompressionMode = CompressionModes[selectedCompModeIdx];
                config.Filter = Filters[selectedFilterIdx];
                config.AddressModeU = AddressModes[selectedAddressModeU];
                config.AddressModeV = AddressModes[selectedAddressModeV];
                config.AddressModeW = AddressModes[selectedAddressModeW];

                ig::AssetManager& assetManager = ig::Engine::GetAssetManager();
                assetManager.Import(path, config);
                return false;
            }
        }

        return true;
    }

    void TextureImportPanel::SelectFileToImport()
    {
        config = {};

        static const std::vector<ig::DialogFilter> Filters{
            ig::DialogFilter{.Name = "Texture Resources"_fs, .FilterPattern = "*.jpg;*.png;*.hdr;*.dds"_fs},
        };

        ig::Result<ig::String, ig::EOpenFileDialogStatus> result = ig::OpenFileDialog::Show(nullptr, "Texture to import"_fs, Filters);
        status = result.GetStatus();
        if (result.HasOwnership())
        {
            path = result.Take();
        }
    }
}    // namespace fe
