#pragma once
#include <Igniter.h>
#include <Filesystem/FileDialog.h>
#include <Asset/Model.h>
#include <ImGui/ImGuiLayer.h>

namespace ig
{
    class StaticMeshImportPanel : public ImGuiLayer
    {
    public:
        StaticMeshImportPanel() = default;
        ~StaticMeshImportPanel() = default;

        void Render() override;
        void OnVisible() override;

    private:
        void SelectFileToImport();

    private:
        String path{};
        StaticMeshImportDesc config;
        EOpenFileDialogStatus status{};
    };
} // namespace ig