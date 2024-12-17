#include "Frieren/Frieren.h"
#include "Igniter/Core/String.h"
#include "Frieren/Gui/CachedStringDebugger.h"

namespace fe
{
    void CachedStringDebugger::OnImGui()
    {
        ImGui::Text("Target ID: ");
        ImGui::SameLine(0.f, 1.f);
        ImGui::InputText("##HashInput", inputBuffer.data(), MaxInputLength, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
        ImGui::SameLine();

        if (ImGui::Button("Search"))
        {
            bFiltered = true;
            searchTargetHashVal = std::stoull(inputBuffer);
        }

        ImGui::SameLine();
        if (ImGui::Button("Reset") || bLoadRequired)
        {
            cachedStrings = ig::String::GetCachedStrings();
            bSortRequired = true;
            bFiltered = false;
            bLoadRequired = false;
            inputBuffer[0] = '\0';
        }

        constexpr ImGuiTableFlags flags =
            ImGuiTableFlags_Reorderable | ImGuiTableFlags_Sortable |
            ImGuiTableFlags_RowBg | ImGuiTableFlags_HighlightHoveredColumn | ImGuiTableFlags_Borders |
            ImGuiTableFlags_ScrollY;

        if (ImGui::BeginTable("Cached Hash-String Table", 2, flags))
        {
            ImGui::TableSetupColumn("Hash", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("String", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
            ImGui::TableHeadersRow();

            if (ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs(); sortSpecs)
            {
                if (sortSpecs->SpecsDirty || bSortRequired)
                {
                    std::sort(cachedStrings.begin(), cachedStrings.end(),
                              [sortSpec = sortSpecs->Specs[0]](const auto& lhs, const auto& rhs)
                    {
                        return (sortSpec.SortDirection == ImGuiSortDirection_Ascending) ? (lhs.first < rhs.first) : (lhs.first >= rhs.first);
                    });

                    sortSpecs->SpecsDirty = false;
                    bSortRequired = false;
                }
            }

            const auto renderElement = [](const std::pair<uint64_t, std::string_view> hashStrPair)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text(std::format("{}", hashStrPair.first).c_str());
                ImGui::TableNextColumn();
                ImGui::Text(hashStrPair.second.data());
            };

            if (bFiltered)
            {
                if (searchTargetHashVal != ig::InvalidHashVal)
                {
                    const auto foundItr = std::find_if(cachedStrings.cbegin(), cachedStrings.cend(),
                                                       [val = searchTargetHashVal](const auto& hashStrPair) { return hashStrPair.first == val; });

                    if (foundItr != cachedStrings.cend())
                    {
                        renderElement(*foundItr);
                    }
                }
            }
            else
            {
                for (const auto& cachedStr : cachedStrings)
                {
                    renderElement(cachedStr);
                }
            }

            ImGui::EndTable();
        }
    }
}    // namespace fe
