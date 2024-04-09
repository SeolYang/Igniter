#include <Igniter.h>
#include <Core/Engine.h>
#include <Core/Timer.h>
#include <ImGui/ImGuiWidgets.h>
#include <ImGui/AssetInspector.h>

namespace ig
{
    AssetInspector::AssetInspector()
    {
        AssetManager& assetManager{ Igniter::GetAssetManager() };
        AssetManager::ModifiedEvent& modifiedEvent{ assetManager.GetModifiedEvent() };
        modifiedEvent.Subscribe("AssetInspector"_fs, [this](const std::reference_wrapper<const AssetManager> assetManagerRef)
        {
            UniqueLock lock{ mutex };
            const AssetManager& assetManager{ assetManagerRef.get() };
            snapshots = assetManager.TakeSnapshots();
            bDirty = true;
            selectedIdx = -1;
            lastUpdated = chrono::system_clock::now();
        });

        snapshots = assetManager.TakeSnapshots();
    }

    void AssetInspector::Render()
    {
        if (ImGui::Begin("Asset Inspector", &bIsVisible, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar))
        {
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("Filters"))
                {
                    selectedTypeFilter = ImGuiX::EnumMenuItems<EAssetType>(selectedTypeFilter, EAssetType::Unknown);
                    if (ImGui::MenuItem("No Filters"))
                    {
                        selectedTypeFilter = std::nullopt;
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            UniqueLock lock{ mutex };
            ImGui::Text(std::format("Last Update at {:%Y/%m/%d %H:%M:%S}", lastUpdated).data());

            if (selectedTypeFilter)
            {
                const auto IsSelected = [selectedTypeFilter = *selectedTypeFilter](const AssetManager::Snapshot& snapshot)
                {
                    return snapshot.Info.GetType() == selectedTypeFilter;
                };

                const auto IsSelectedCached = [selectedTypeFilter = *selectedTypeFilter](const AssetManager::Snapshot& snapshot)
                {
                    return snapshot.Info.GetType() == selectedTypeFilter && snapshot.IsCached();
                };

                ImGui::Text(std::format("#Imported {}: {}\t#Cached {}: {}",
                            *selectedTypeFilter,
                            std::count_if(snapshots.begin(), snapshots.end(), IsSelected),
                            *selectedTypeFilter,
                            std::count_if(snapshots.begin(), snapshots.end(), IsSelectedCached))
                            .c_str());
            }
            else
            {
                const auto IsCached = [](const AssetManager::Snapshot& snapshot)
                {
                    return snapshot.IsCached();
                };

                ImGui::Text(std::format("#Imported Assets: {}\t#Cached Assets: {}", snapshots.size(),
                            std::count_if(snapshots.begin(), snapshots.end(), IsCached))
                            .c_str());
            }

            constexpr ImGuiTableFlags TableFlags =
                ImGuiTableFlags_Reorderable |
                ImGuiTableFlags_Sortable |
                ImGuiTableFlags_RowBg |
                ImGuiTableFlags_Borders |
                ImGuiTableFlags_ScrollY |
                ImGuiTableFlags_ScrollX |
                ImGuiTableFlags_ContextMenuInBody |
                ImGuiTableFlags_HighlightHoveredColumn;

            constexpr uint8_t NumColumns{ 5 };

            ImGui::BeginChild("Table", ImVec2{ 300, 0 }, ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX);
            if (ImGui::BeginTable("Assets", NumColumns, TableFlags))
            {
                constexpr ImGuiTableColumnFlags ColumnFlags = ImGuiTableColumnFlags_WidthFixed;
                ImGui::TableSetupColumn("Type", ColumnFlags | ImGuiTableColumnFlags_DefaultSort);
                ImGui::TableSetupColumn("Guid", ColumnFlags);
                ImGui::TableSetupColumn("Virtual Path", ColumnFlags);
                ImGui::TableSetupColumn("Persistency", ColumnFlags);
                ImGui::TableSetupColumn("Refs", ColumnFlags | ImGuiTableColumnFlags_PreferSortDescending);
                ImGui::TableHeadersRow();

                ImGuiTableSortSpecs* sortSpecs{ ImGui::TableGetSortSpecs() };
                if (sortSpecs->SpecsDirty || bDirty)
                {
                    IG_CHECK(sortSpecs->SpecsCount > 0);
                    const ImGuiSortDirection sortDirection{ sortSpecs->Specs[0].SortDirection };
                    const int16_t selectedColumn{ sortSpecs->Specs[0].ColumnIndex };
                    const bool bAscendingRequired{ sortDirection == ImGuiSortDirection_Ascending };
                    std::sort(snapshots.begin(), snapshots.end(),
                              [selectedColumn, bAscendingRequired](const AssetManager::Snapshot& lhs, const AssetManager::Snapshot& rhs)
                    {
                        int comp{};
                        switch (selectedColumn)
                        {
                            case 0:
                            return bAscendingRequired ? lhs.Info.GetType() < rhs.Info.GetType() :
                                lhs.Info.GetType() > rhs.Info.GetType();

                            case 1:
                            return bAscendingRequired ? lhs.Info.GetGuid().bytes() < rhs.Info.GetGuid().bytes() :
                                lhs.Info.GetGuid().bytes() > rhs.Info.GetGuid().bytes();
                            case 2:
                            comp = lhs.Info.GetVirtualPath().ToStringView().compare(rhs.Info.GetVirtualPath().ToStringView());
                            return bAscendingRequired ? comp < 0 : comp > 0;

                            case 3:
                            return bAscendingRequired ? lhs.Info.GetScope() < rhs.Info.GetScope() :
                                lhs.Info.GetScope() > rhs.Info.GetScope();
                            case 4:
                            return bAscendingRequired ? lhs.RefCount < rhs.RefCount :
                                lhs.RefCount > rhs.RefCount;

                            default:
                            IG_CHECK_NO_ENTRY();
                            return false;
                        };
                    });

                    sortSpecs->SpecsDirty = false;
                    bDirty = false;
                }
                // Type // GUID // VIRTUAL PATH // PERSISTENCY // REF COUNT

                for (int idx = 0; idx < snapshots.size(); ++idx)
                {
                    const AssetManager::Snapshot& snapshot{ snapshots[idx] };
                    const EAssetType assetType{ snapshot.Info.GetType() };
                    if (!selectedTypeFilter || *selectedTypeFilter == assetType)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Text(ToCStr(snapshot.Info.GetType()));
                        ImGui::TableNextColumn();
                        if (ImGui::Selectable(std::format("{}", snapshot.Info.GetGuid()).c_str(), (selectedIdx == idx), ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap))
                        {
                            selectedIdx = idx;
                            selectedSnapshot = snapshots[selectedIdx];
                        }

                        ImGui::TableNextColumn();
                        ImGui::Text(snapshot.Info.GetVirtualPath().ToCString());
                        ImGui::TableNextColumn();
                        ImGui::Text(ToCStr(snapshot.Info.GetScope()));
                        ImGui::TableNextColumn();
                        ImGui::Text("%u", snapshot.RefCount);
                    }
                }
                ImGui::EndTable();
            }
            ImGui::EndChild();

            ImGui::SameLine();
            ImGui::BeginChild("Edit");
            ImGui::SeparatorText("Asset Informations");
            if (selectedIdx != -1)
            {
                AssetInfo& assetInfo{ selectedSnapshot.Info };
                ImGui::TextDisabled(std::format("Type: {}", assetInfo.GetType()).c_str());
                ImGui::TextDisabled(std::format("GUID: {}", assetInfo.GetGuid()).c_str());
                ImGui::TextDisabled(std::format("Virtual Path: {}", assetInfo.GetVirtualPath()).c_str());
                ImGui::TextDisabled(std::format("Scope: {}", assetInfo.GetScope()).c_str());
            }
            ImGui::EndChild();

            ImGui::End();
        }
    }
} // namespace ig