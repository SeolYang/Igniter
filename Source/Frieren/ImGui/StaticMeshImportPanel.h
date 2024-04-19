#pragma once
#include <Frieren.h>
#include <Filesystem/FileDialog.h>
#include <Asset/StaticMesh.h>
#include <ImGui/ImGuiLayer.h>

namespace fe
{
    class StaticMeshImportPanel final : public ImGuiLayer
    {
    public:
        StaticMeshImportPanel()                                 = default;
        StaticMeshImportPanel(const StaticMeshImportPanel&)     = delete;
        StaticMeshImportPanel(StaticMeshImportPanel&&) noexcept = delete;
        ~StaticMeshImportPanel() override                       = default;

        StaticMeshImportPanel& operator=(const StaticMeshImportPanel&)     = delete;
        StaticMeshImportPanel& operator=(StaticMeshImportPanel&&) noexcept = delete;

        void Render() override;
        void OnVisible() override;

    private:
        void SelectFileToImport();

    private:
        String                path{};
        StaticMeshImportDesc  config;
        EOpenFileDialogStatus status{};
    };
} // namespace ig
