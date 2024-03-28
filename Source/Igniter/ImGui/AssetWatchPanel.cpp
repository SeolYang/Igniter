#include <Igniter.h>
#include <Core/Result.h>
#include <ImGui/AssetWatchPanel.h>

namespace ig
{
    AssetWatchPanel::AssetWatchPanel() : watcher{ "Assets"_fs, ig::EFileWatchFilterFlags::ChangeLastWrite | ig::EFileWatchFilterFlags::ChangeFileName }
    {
        infoBuffer.reserve(64);
    }

    void AssetWatchPanel::Render()
    {
        if (ImGui::Begin("Asset Watcher", &bIsVisible))
        {
            if (ImGui::Button("Clear"))
            {
                infoBuffer.clear();
            }

            ImGui::SameLine();

            constexpr ImGuiTableFlags TableFlags =
                ImGuiTableFlags_Reorderable |
                ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersOuter |
                ImGuiTableFlags_BordersV |
                ImGuiTableFlags_NoBordersInBody |
                ImGuiTableFlags_ScrollY;

            if (ImGui::BeginTable("Latest Changes", 6, TableFlags))
            {
                ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Creation", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Modified", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Changed", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableHeadersRow();

                for (const auto& changeInfo : infoBuffer)
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", magic_enum::enum_name(changeInfo.Action).data());
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", changeInfo.Path.string().c_str());
                    ImGui::TableNextColumn();
                    ImGui::Text("%llu", changeInfo.FileSize);
                    ImGui::TableNextColumn();
                    ImGui::Text("%llu", changeInfo.CreationTime);
                    ImGui::TableNextColumn();
                    ImGui::Text("%llu", changeInfo.LastModificationTime);
                    ImGui::TableNextColumn();
                    ImGui::Text("%llu", changeInfo.LastChangeTime);
                }
                ImGui::EndTable();
            }
            ImGui::End();
        }

        for (const auto& newChangeInfo : watcher.RequestChanges(false))
        {
            infoBuffer.emplace_back(newChangeInfo);
        }
    }
} // namespace ig