#pragma once
#include <Frieren.h>
#include <Filesystem/CoFileWatcher.h>
#include <ImGui/ImGuiLayer.h>

namespace fe
{
    class AssetWatchPanel final : public ImGuiLayer
    {
    public:
        AssetWatchPanel();
        AssetWatchPanel(const AssetWatchPanel&) = delete;
        AssetWatchPanel(AssetWatchPanel&&) noexcept = delete;
        ~AssetWatchPanel() override = default;

        AssetWatchPanel& operator=(const AssetWatchPanel&) = delete;
        AssetWatchPanel& operator=(AssetWatchPanel&&) noexcept = delete;

        void Render() override;

    private:
        CoFileWatcher watcher;
        std::vector<FileChangeInfo> infoBuffer;
    };
}    // namespace fe
