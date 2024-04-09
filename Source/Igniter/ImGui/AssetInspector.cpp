#include <Igniter.h>
#include <Core/Engine.h>
#include <Core/Timer.h>
#include <D3D12/GpuBuffer.h>
#include <D3D12/GpuTexture.h>
#include <Render/GpuViewManager.h>
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
            RecursiveLock lock{ mutex };
            const AssetManager& assetManager{ assetManagerRef.get() };
            snapshots = assetManager.TakeSnapshots();
            bDirty = true;
            mainTableSelectedIdx = -1;
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
                    std::optional<EAssetType> selectedFilter = ImGuiX::EnumMenuItems<EAssetType>(mainTableAssetFilter, EAssetType::Unknown);
                    if (ImGui::MenuItem("No Filters") && !selectedFilter)
                    {
                        mainTableAssetFilter = EAssetType::Unknown;
                    }
                    else if (selectedFilter)
                    {
                        mainTableAssetFilter = *selectedFilter;
                        mainTableSelectedIdx = -1;
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            RecursiveLock lock{ mutex };
            ImGui::Text(std::format("Last Update at {:%Y/%m/%d %H:%M:%S}", lastUpdated).data());

            if (mainTableAssetFilter != EAssetType::Unknown)
            {
                const auto IsSelected = [selectedTypeFilter = mainTableAssetFilter](const AssetManager::Snapshot& snapshot)
                {
                    return snapshot.Info.GetType() == selectedTypeFilter;
                };

                const auto IsSelectedCached = [selectedTypeFilter = mainTableAssetFilter](const AssetManager::Snapshot& snapshot)
                {
                    return snapshot.Info.GetType() == selectedTypeFilter && snapshot.IsCached();
                };

                ImGui::Text(std::format("#Imported {}: {}\t#Cached {}: {}",
                            mainTableAssetFilter,
                            std::count_if(snapshots.begin(), snapshots.end(), IsSelected),
                            mainTableAssetFilter,
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



            ImGui::BeginChild("Table", ImVec2{ 300, 0 }, ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX);
            ImGui::SeparatorText("Assets");
            RenderAssetTable(mainTableAssetFilter, mainTableSelectedIdx);
            ImGui::EndChild();

            ImGui::SameLine();
            ImGui::BeginChild("Edit");
            ImGui::SeparatorText("Inspector");
            if (mainTableSelectedIdx != -1)
            {
                RenderEdit();
            }
            ImGui::EndChild();

            ImGui::End();
        }
    }

    void AssetInspector::RenderAssetTable(const EAssetType assetTypeFilter, int& selectedIdx)
    {
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
                if (assetTypeFilter == EAssetType::Unknown || assetTypeFilter == assetType)
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text(ToCStr(snapshot.Info.GetType()));
                    ImGui::TableNextColumn();
                    if (ImGui::Selectable(std::format("{}", snapshot.Info.GetGuid()).c_str(), (selectedIdx == idx), ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap))
                    {
                        selectedIdx = idx;
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
    }

    void AssetInspector::RenderEdit()
    {
        AssetManager::Snapshot& selectedSnapshot{ snapshots[mainTableSelectedIdx] };
        const AssetInfo& assetInfo{ selectedSnapshot.Info };
        RenderAssetInfo(assetInfo);

        if (assetInfo.GetScope() != EAssetScope::Engine)
        {
            switch (assetInfo.GetType())
            {
                case EAssetType::Material:
                RenderMaterialEdit(assetInfo);
                break;
                case EAssetType::StaticMesh:
                RenderStaticMeshEdit(assetInfo);
                break;
            }
        }
    }

    void AssetInspector::RenderMaterialEdit(const AssetInfo& assetInfo)
    {
        AssetManager& assetManager{ Igniter::GetAssetManager() };
        std::optional<Material::LoadDesc> loadDescOpt{ assetManager.GetLoadDesc<Material>(assetInfo.GetGuid()) };
        ImGui::SeparatorText("Material");
        if (!loadDescOpt)
        {
            ImGui::Text("Invalid Material!");
            return;
        }

        Material::LoadDesc loadDesc{ *loadDescOpt };
        RenderSelector("Diffuse", loadDesc.DiffuseTexGuid);
        int newDiffuseIdx{ RenderSelectorPopup(EAssetType::Texture) };
        if (newDiffuseIdx != -1)
        {
            const AssetInfo& newDiffuseInfo{ snapshots[newDiffuseIdx].Info };
            IG_CHECK(newDiffuseInfo.IsValid());
            IG_CHECK(newDiffuseInfo.GetType() == EAssetType::Texture);
            loadDesc.DiffuseTexGuid = newDiffuseInfo.GetGuid();
            assetManager.UpdateLoadDesc<Material>(assetInfo.GetGuid(), loadDesc);
            assetManager.Reload<Material>(assetInfo.GetGuid());
        }
    }

    void AssetInspector::RenderStaticMeshEdit(const AssetInfo& assetInfo)
    {
        AssetManager& assetManager{ Igniter::GetAssetManager() };
        std::optional<StaticMesh::LoadDesc> loadDescOpt{ assetManager.GetLoadDesc<StaticMesh>(assetInfo.GetGuid()) };
        ImGui::SeparatorText("Static Mesh");
        if (!loadDescOpt)
        {
            ImGui::Text("Invalid Static Mesh!");
            return;
        }

        StaticMesh::LoadDesc loadDesc{ *loadDescOpt };
        RenderSelector("Material", loadDesc.MaterialGuid);
        int newMaterialIdx{ RenderSelectorPopup(EAssetType::Material) };
        if (newMaterialIdx != -1)
        {
            const AssetInfo& newMatInfo{ snapshots[newMaterialIdx].Info };
            IG_CHECK(newMatInfo.IsValid());
            IG_CHECK(newMatInfo.GetType() == EAssetType::Material);
            loadDesc.MaterialGuid = newMatInfo.GetGuid();
            assetManager.UpdateLoadDesc<StaticMesh>(assetInfo.GetGuid(), loadDesc);
            assetManager.Reload<StaticMesh>(assetInfo.GetGuid());
        }
    }

    void AssetInspector::RenderAssetInfo(const AssetInfo& assetInfo)
    {
        ImGui::TextDisabled(std::format("Type: {}", assetInfo.GetType()).c_str());
        ImGui::TextDisabled(std::format("GUID: {}", assetInfo.GetGuid()).c_str());
        ImGui::TextDisabled(std::format("Virtual Path: {}", assetInfo.GetVirtualPath()).c_str());
        ImGui::TextDisabled(std::format("Scope: {}", assetInfo.GetScope()).c_str());
    }

    void AssetInspector::RenderSelector(const char* label, const Guid guid)
    {
        IG_CHECK(label != nullptr);
        if (!ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen))
        {
            return;
        }

        AssetManager& assetManager{ Igniter::GetAssetManager() };
        std::optional<AssetInfo> assetInfoOpt{ assetManager.GetAssetInfo(guid) };
        if (!assetInfoOpt)
        {
            return;
        }

        const AssetInfo& assetInfo{ *assetInfoOpt };
        RenderAssetInfo(assetInfo);
        if (!ImGui::Button("Select"))
        {
            return;
        }

        bOpenSelectorPopup = true;
    }

    int AssetInspector::RenderSelectorPopup(const EAssetType selectAssetType)
    {
        int selectedIdx = -1;
        if (bOpenSelectorPopup)
        {
            selectorTableSelectedIdx = -1;
            ImGui::OpenPopup(std::format("{} selector", selectAssetType).c_str());
            bOpenSelectorPopup = false;
        }

        if (ImGui::BeginPopupModal(std::format("{} selector", selectAssetType).c_str()))
        {
            if (ImGui::Button("Select"))
            {
                selectedIdx = selectorTableSelectedIdx;
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::Button("Close"))
            {
                ImGui::CloseCurrentPopup();
            }

            RenderAssetTable(selectAssetType, selectorTableSelectedIdx);
            ImGui::EndPopup();
        }

        return selectedIdx;
    }
} // namespace ig