#pragma once
#include <Frieren.h>
#include <Filesystem/FileDialog.h>
#include <Asset/Texture.h>
#include <ImGui/ImGuiLayer.h>

namespace fe
{
    class TextureImportPanel final : public ImGuiLayer
    {
    public:
        TextureImportPanel()                          = default;
        TextureImportPanel(const TextureImportPanel&) = delete;
        TextureImportPanel(TextureImportPanel&&)      = delete;
        ~TextureImportPanel() override                = default;

        TextureImportPanel& operator=(const TextureImportPanel&)     = delete;
        TextureImportPanel& operator=(TextureImportPanel&&) noexcept = delete;

        void Render() override;
        void OnVisible() override;

    private:
        void SelectFileToImport();

    private:
        String                path{};
        TextureImportDesc     config;
        EOpenFileDialogStatus status{};

        int selectedCompModeIdx  = 0;
        int selectedFilterIdx    = 0;
        int selectedAddressModeU = 0;
        int selectedAddressModeV = 0;
        int selectedAddressModeW = 0;
    };
} // namespace ig
