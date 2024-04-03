#include <Igniter.h>
#include <Core/Engine.h>
#include <Core/Timer.h>
#include <ImGui/ImGuiWidgets.h>
#include <ImGui/AssetSnapshotPanel.h>

namespace ig
{
    AssetSnapshotPanel::AssetSnapshotPanel()
    {
        AssetManager& assetManager{ Igniter::GetAssetManager() };
        AssetManager::ModifiedEvent& modifiedEvent{ assetManager.GetModifiedEvent() };
        modifiedEvent.Subscribe("AssetSnapshotPanel"_fs, [this](const std::reference_wrapper<const AssetManager> assetManagerRef)
                                {
                                    UniqueLock lock{ mutex };
                                    const AssetManager& assetManager{ assetManagerRef.get() };
                                    snapshots = assetManager.TakeSnapshots();
                                    bDirty = true;
                                    lastUpdated = chrono::system_clock::now();
                                });

        snapshots = assetManager.TakeSnapshots();
    }

    void AssetSnapshotPanel::Render()
    {
        if (ImGui::Begin("Asset Snapshots Panel", &bIsVisible, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar))
        {
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("Filters"))
                {
                    selectedTypeFilter = EnumMenuItems<EAssetType>(selectedTypeFilter, EAssetType::Unknown);
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
                ImGuiTableFlags_HighlightHoveredColumn;

            constexpr uint8_t NumColumns{ 5 };
            if (ImGui::BeginTable("Asset Snapshots", NumColumns, TableFlags))
            {
                constexpr ImGuiTableColumnFlags ColumnFlags = ImGuiTableColumnFlags_WidthFixed;
                ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_DefaultSort | ColumnFlags);
                ImGui::TableSetupColumn("Guid", ColumnFlags);
                ImGui::TableSetupColumn("Virtual Path", ColumnFlags);
                ImGui::TableSetupColumn("Persistency", ColumnFlags);
                ImGui::TableSetupColumn("Refs", ColumnFlags);
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
                                          return bAscendingRequired ? lhs.Info.GetPersistency() < rhs.Info.GetPersistency() :
                                                                      lhs.Info.GetPersistency() > rhs.Info.GetPersistency();
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

                for (const AssetManager::Snapshot& snapshot : snapshots)
                {
                    const EAssetType assetType{ snapshot.Info.GetType() };
                    if (!selectedTypeFilter || *selectedTypeFilter == assetType)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Text(ToCStr(snapshot.Info.GetType()));
                        ImGui::TableNextColumn();
                        ImGui::Text(snapshot.Info.GetGuid().str().c_str());
                        ImGui::TableNextColumn();
                        ImGui::Text(snapshot.Info.GetVirtualPath().ToCString());
                        ImGui::TableNextColumn();
                        ImGui::Text(ToCStr(snapshot.Info.GetPersistency()));
                        ImGui::TableNextColumn();
                        ImGui::Text("%u", snapshot.RefCount);
                    }
                }

                ImGui::EndTable();
            }

            ImGui::End();
        }
    }
} // namespace ig