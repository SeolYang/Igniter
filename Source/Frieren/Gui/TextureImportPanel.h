#pragma once
#include "Frieren/Frieren.h"
#include "Igniter/Filesystem/FileDialog.h"
#include "Igniter/Asset/Texture.h"

namespace fe
{
    class TextureImportPanel final
    {
    public:
        TextureImportPanel() = default;
        TextureImportPanel(const TextureImportPanel&) = delete;
        TextureImportPanel(TextureImportPanel&&) = delete;
        ~TextureImportPanel() = default;

        TextureImportPanel& operator=(const TextureImportPanel&) = delete;
        TextureImportPanel& operator=(TextureImportPanel&&) noexcept = delete;

        bool OnImGui();
        void SelectFileToImport();

    private:
        ig::String path{};
        ig::TextureImportDesc config;
        ig::EOpenFileDialogStatus status{};

        int selectedCompModeIdx = 0;
        int selectedFilterIdx = 0;
        int selectedAddressModeU = 0;
        int selectedAddressModeV = 0;
        int selectedAddressModeW = 0;
    };
}    // namespace fe
