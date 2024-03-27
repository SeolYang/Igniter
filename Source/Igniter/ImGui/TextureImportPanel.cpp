#include <Igniter.h>
#include <Core/Engine.h>
#include <Core/Log.h>
#include <Asset/AssetManager.h>
#include <ImGui/TextureImportPanel.h>
#include <ImGui/ImGuiUtils.h>

IG_DEFINE_LOG_CATEGORY(TextureImportPanel);

namespace ig
{
    TextureImportPanel::~TextureImportPanel()
    {
    }

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
                ImGui::Text("Failed to select texture resource file. (Status: {})", magic_enum::enum_name(status).data());
            }
            else
            {
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
                    Result<xg::Guid, EAssetImportResult> result = assetManager.ImportTexture(path, config);
                    if (result.HasOwnership())
                    {
                        const xg::Guid importedGuid = result.Take();
                        IG_CHECK(importedGuid.isValid());
                        IG_LOG(TextureImportPanel, Info, "Texture {} imported as {}.", path.ToStringView(), importedGuid.str());
                    }
                    else
                    {
                        IG_LOG(TextureImportPanel, Error, "Failed to import texture {}. Status: {}", path.ToStringView(), magic_enum::enum_name(result.GetStatus()));
                    }

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
        static const std::vector<DialogFilter> Filters{
            DialogFilter{ .Name = "JPEG Files"_fs, .FilterPattern = "*.jpg"_fs },
            DialogFilter{ .Name = "PNG Files"_fs, .FilterPattern = "*.png"_fs }
        };

        Result<String, EOpenFileDialogStatus> result = OpenFileDialog::Show(nullptr, "Texture to import"_fs, Filters);
        status = result.GetStatus();
        if (result.HasOwnership())
        {
            path = result.Take();
        }
    }

} // namespace ig