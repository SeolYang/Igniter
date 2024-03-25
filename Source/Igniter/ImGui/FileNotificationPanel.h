#pragma once
#include <Igniter.h>
#include <Filesystem/AsyncFileTracker.h>
#include <ImGui/ImGuiLayer.h>

namespace ig
{
    class FileNotificationPanel : public ImGuiLayer
    {
    public:
        FileNotificationPanel();
        ~FileNotificationPanel() = default;

        void Render() override;

    private:
        AsyncFileTracker fileTracker{};
        EFileTrackerStatus lastTrackingStatus = EFileTrackerStatus::FileDoesNotExist;
        std::string targetDirPath;

        std::vector<FileNotification> notifications;
    };

} // namespace ig