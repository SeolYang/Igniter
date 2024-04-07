#include <Igniter.h>
#include <Core/Engine.h>
#include <Asset/AssetManager.h>
#include <ImGui/TextureImportPanel.h>
#include <ImGui/ImGuiWidgets.h>

namespace ig
{
    void TextureImportPanel::Render()
    {
        if (ImGui::Begin("Texture Import", &bIsVisible))
        {
            if (ImGui::Button("Reselect"))
            {
                SelectFileToImport();
            }

            if (status != EOpenFileDialogStatus::Success)
            {
                ImGui::Text("Failed to select texture resource file. (Status: %s)", magic_enum::enum_name(status).data());
            }
            else
            {
                ImGui::SameLine();
                ImGui::Text("%s", path.ToStringView().data());

                if (BeginEnumCombo<ETextureCompressionMode>("Compression Mode", selectedCompModeIdx))
                {
                    EndEnumCombo();
                }

                ImGui::Checkbox("Generate Mips", &config.bGenerateMips);

                if (BeginEnumCombo<D3D12_FILTER>("Filter", selectedFilterIdx))
                {
                    EndEnumCombo();
                }

                if (BeginEnumCombo<D3D12_TEXTURE_ADDRESS_MODE>("Address Mode(U)", selectedAddressModeU))
                {
                    EndEnumCombo();
                }

                if (BeginEnumCombo<D3D12_TEXTURE_ADDRESS_MODE>("Address Mode(V)", selectedAddressModeV))
                {
                    EndEnumCombo();
                }

                if (BeginEnumCombo<D3D12_TEXTURE_ADDRESS_MODE>("Address Mode(W)", selectedAddressModeW))
                {
                    EndEnumCombo();
                }

                if (ImGui::Button("Import"))
                {
                    constexpr auto CompressionModes = magic_enum::enum_values<ETextureCompressionMode>();
                    constexpr auto Filters = magic_enum::enum_values<D3D12_FILTER>();
                    constexpr auto AddressModes = magic_enum::enum_values<D3D12_TEXTURE_ADDRESS_MODE>();

                    config.CompressionMode = CompressionModes[selectedCompModeIdx];
                    config.Filter = Filters[selectedFilterIdx];
                    config.AddressModeU = AddressModes[selectedAddressModeU];
                    config.AddressModeV = AddressModes[selectedAddressModeV];
                    config.AddressModeW = AddressModes[selectedAddressModeW];

                    AssetManager& assetManager = Igniter::GetAssetManager();
                    assetManager.Import(path, config);
                    SetVisibility(false);
                }
            }

            ImGui::End();
        }
    }

    void TextureImportPanel::OnVisible()
    {
        SelectFileToImport();
    }

    void TextureImportPanel::SelectFileToImport()
    {
        config = {};

        static const std::vector<DialogFilter> Filters{
            DialogFilter{ .Name = "JPEG Files"_fs, .FilterPattern = "*.jpg"_fs },
            DialogFilter{ .Name = "PNG Files"_fs, .FilterPattern = "*.png"_fs },
            DialogFilter{ .Name = "HDR Files"_fs, .FilterPattern = "*.hdr"_fs },
            DialogFilter{ .Name = "DDS Files"_fs, .FilterPattern = "*.dds"_fs }
        };

        Result<String, EOpenFileDialogStatus> result = OpenFileDialog::Show(nullptr, "Texture to import"_fs, Filters);
        status = result.GetStatus();
        if (result.HasOwnership())
        {
            path = result.Take();
        }
    }

} // namespace ig