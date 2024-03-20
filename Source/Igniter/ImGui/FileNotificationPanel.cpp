#include <ImGui/FileNotificationPanel.h>
#include <Core/Result.h>

namespace ig
{
    FileNotificationPanel::FileNotificationPanel()
    {
        targetDirPath.reserve(MAX_PATH);
        notifications.reserve(64);
    }

    void FileNotificationPanel::Render()
    {
        if (ImGui::Begin("File Notifications", &bIsVisible))
        {
            if (ImGui::Button("Clear"))
            {
                notifications.clear();
            }

            ImGui::SameLine();
            if (ImGui::InputText("Directory Path", &targetDirPath))
            {
                if (fileTracker.IsTracking())
                {
                    fileTracker.StopTracking();
                }

                lastTrackingStatus = fileTracker.StartTracking(targetDirPath);
            }

            if (IsSucceeded(lastTrackingStatus))
            {
                for (const auto& notification : fileTracker)
                {
                    notifications.emplace_back(notification);
                }
            }

            ImGui::Text("File Tracker Status: %s", magic_enum::enum_name(lastTrackingStatus).data());

            constexpr ImGuiTableFlags TableFlags =
                ImGuiTableFlags_Reorderable |
                ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersOuter |
                ImGuiTableFlags_BordersV |
                ImGuiTableFlags_NoBordersInBody |
                ImGuiTableFlags_ScrollY;

            if (ImGui::BeginTable("Latest Notifications", 2, TableFlags))
            {
                ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableHeadersRow();

                for (const auto& notification : notifications)
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", magic_enum::enum_name(notification.Action).data());
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", notification.Path.string().c_str());
                }
                ImGui::EndTable();
            }
            ImGui::End();
        }
    }
} // namespace ig