#pragma once
#include <Igniter.h>
#include <Filesystem/CoFileWatcher.h>
#include <ImGui/ImGuiLayer.h>

namespace ig
{
    class AssetWatchPanel : public ImGuiLayer
    {
    public:
        AssetWatchPanel();
        ~AssetWatchPanel() = default;

        void Render() override;

    private:
        CoFileWatcher               watcher;
        std::vector<FileChangeInfo> infoBuffer;
    };
} // namespace ig
