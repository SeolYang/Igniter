#pragma once
#include <ImGui/ImGuiLayer.h>
#include <Filesystem/AsyncFileTracker.h>

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
        EFileTrackerResult lastTrackingStatus = EFileTrackerResult::FileDoesNotExist;
        std::string targetDirPath;
        std::vector<FileNotification> notifications;
    };

} // namespace ig