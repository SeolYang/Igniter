#pragma once
#include <Frieren.h>
#include <Filesystem/FileDialog.h>
#include <Asset/StaticMesh.h>

namespace fe
{
    class StaticMeshImportPanel final
    {
    public:
        StaticMeshImportPanel() = default;
        StaticMeshImportPanel(const StaticMeshImportPanel&) = delete;
        StaticMeshImportPanel(StaticMeshImportPanel&&) noexcept = delete;
        ~StaticMeshImportPanel() = default;

        StaticMeshImportPanel& operator=(const StaticMeshImportPanel&) = delete;
        StaticMeshImportPanel& operator=(StaticMeshImportPanel&&) noexcept = delete;

        bool OnImGui();
        void SelectFileToImport();

    private:
        String path{};
        StaticMeshImportDesc config;
        EOpenFileDialogStatus status{};
    };
}    // namespace fe
