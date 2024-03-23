#pragma once
#include <Core/Mutex.h>
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
        void OnNotification(const FileNotification& newNotification);

    private:
        AsyncFileTracker fileTracker{};
        EFileTrackerResult lastTrackingStatus = EFileTrackerResult::FileDoesNotExist;
        std::string targetDirPath;

        SharedMutex mutex;
        std::vector<FileNotification> notifications;
    };

} // namespace ig