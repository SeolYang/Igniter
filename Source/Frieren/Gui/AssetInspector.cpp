#include "Frieren/Frieren.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Core/Timer.h"
#include "Igniter/Core/FrameManager.h"
#include "Igniter/D3D12/GpuBuffer.h"
#include "Igniter/D3D12/GpuTextureDesc.h"
#include "Igniter/D3D12/GpuTexture.h"
#include "Igniter/D3D12/GpuView.h"
#include "Igniter/D3D12/GpuDevice.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/ImGui/ImGuiExtensions.h"
#include "Frieren/Gui/AssetInspector.h"

namespace fe
{
    AssetInspector::AssetInspector()
    {
        ig::AssetManager& assetManager{ig::Engine::GetAssetManager()};
        ig::AssetManager::ModifiedEvent& modifiedEvent{assetManager.GetModifiedEvent()};
        modifiedEvent.Subscribe("AssetInspector"_fs,
                                [this]([[maybe_unused]] const ig::CRef<ig::AssetManager> assetManagerRef)
                                {
                                    bIsDirty = true;
                                });

        snapshots = assetManager.TakeSnapshots();
    }

    AssetInspector::~AssetInspector()
    {
        if (previewTexture)
        {
            ig::AssetManager& assetManager = ig::Engine::GetAssetManager();
            assetManager.Unload(previewTexture);
        }
    }

    void AssetInspector::OnImGui()
    {
        if (bIsDirty)
        {
            const ig::AssetManager& assetManager{ig::Engine::GetAssetManager()};
            snapshots = assetManager.TakeSnapshots();
            lastUpdated = ig::chrono::system_clock::now();
        }

        RenderMenuBar();
        RenderMainFrame();
    }

    void AssetInspector::RenderMenuBar()
    {
        if (ImGui::BeginMenuBar())
        {
            RenderFilterMenu();
            ImGui::EndMenuBar();
        }
    }

    void AssetInspector::RenderFilterMenu()
    {
        if (ImGui::BeginMenu("Filters"))
        {
            std::optional<ig::EAssetCategory> selectedFilter =
                ig::ImGuiX::EnumMenuItems<ig::EAssetCategory>(mainTableAssetFilter, ig::EAssetCategory::Unknown);
            if (ImGui::MenuItem("No Filters") && !selectedFilter)
            {
                mainTableAssetFilter = ig::EAssetCategory::Unknown;
            }
            else if (selectedFilter)
            {
                mainTableAssetFilter = *selectedFilter;
                mainTableSelectedIdx = -1;
            }
            ImGui::EndMenu();
        }
    }

    void AssetInspector::RenderMainFrame()
    {
        RenderAssetStats();

        constexpr float MainTableRatio = 0.6f;
        constexpr float InspectorRatio = 1.f - MainTableRatio;

        const float width{ImGui::GetContentRegionAvail().x};

        ImGui::BeginGroup();
        ImGui::BeginChild("Table", ImVec2{width * MainTableRatio, 0.f}, ImGuiChildFlags_Border);
        ig::ImGuiX::SeparatorText("Assets");
        RenderAssetTable(mainTableAssetFilter, mainTableSelectedIdx, &bIsMainSelectionDirty);
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("Inspector", ImVec2{width * InspectorRatio - ig::ImGuiX::GetFramePadding().x, 0.f}, ImGuiChildFlags_Border,
                          ImGuiWindowFlags_HorizontalScrollbar);
        RenderInspector();
        ImGui::EndChild();
        ImGui::EndGroup();
    }

    void AssetInspector::RenderAssetStats()
    {
        ImGui::Text(std::format("Last Update at {:%Y/%m/%d %H:%M:%S}", lastUpdated).data());
        if (mainTableAssetFilter != ig::EAssetCategory::Unknown)
        {
            const auto IsSelected = [selectedTypeFilter = mainTableAssetFilter](const ig::AssetManager::Snapshot& snapshot)
            {
                return snapshot.Info.GetCategory() == selectedTypeFilter;
            };

            const auto IsSelectedCached = [selectedTypeFilter = mainTableAssetFilter](const ig::AssetManager::Snapshot& snapshot)
            {
                return snapshot.Info.GetCategory() == selectedTypeFilter && snapshot.IsCached();
            };

            ImGui::Text(
                std::format("#Imported {}: {}\t#Cached {}: {}", mainTableAssetFilter, std::count_if(snapshots.begin(), snapshots.end(), IsSelected),
                            mainTableAssetFilter, std::count_if(snapshots.begin(), snapshots.end(), IsSelectedCached))
                    .c_str());
        }
        else
        {
            const auto IsCached = [](const ig::AssetManager::Snapshot& snapshot)
            { return snapshot.IsCached(); };

            ImGui::Text(
                std::format("#Imported Assets: {}\t#Cached Assets: {}", snapshots.size(), std::count_if(snapshots.begin(), snapshots.end(), IsCached))
                    .c_str());
        }
    }

    void AssetInspector::RenderAssetTable(const ig::EAssetCategory assetCategoryFilter, int& selectedIdx, bool* bSelectionDirtyFlagPtr)
    {
        constexpr ImGuiTableFlags TableFlags =
            ImGuiTableFlags_Reorderable | ImGuiTableFlags_Sortable | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX |
            ImGuiTableFlags_ContextMenuInBody | ImGuiTableFlags_HighlightHoveredColumn;

        constexpr uint8_t NumColumns{5};

        if (ImGui::BeginTable("Assets", NumColumns, TableFlags))
        {
            constexpr ImGuiTableColumnFlags ColumnFlags = ImGuiTableColumnFlags_WidthFixed;
            ImGui::TableSetupColumn("Category", ColumnFlags | ImGuiTableColumnFlags_DefaultSort);
            ImGui::TableSetupColumn("Guid", ColumnFlags);
            ImGui::TableSetupColumn("Virtual Path", ColumnFlags);
            ImGui::TableSetupColumn("Scope", ColumnFlags);
            ImGui::TableSetupColumn("Refs", ColumnFlags | ImGuiTableColumnFlags_PreferSortDescending);
            ImGui::TableSetupScrollFreeze(1, 1);
            ImGui::TableHeadersRow();

            ImGuiTableSortSpecs* sortSpecs{ImGui::TableGetSortSpecs()};
            if (sortSpecs->SpecsDirty || bIsDirty)
            {
                IG_CHECK(sortSpecs->SpecsCount > 0);
                const ImGuiSortDirection sortDirection{sortSpecs->Specs[0].SortDirection};
                const int16_t selectedColumn{sortSpecs->Specs[0].ColumnIndex};
                const bool bAscendingRequired{sortDirection == ImGuiSortDirection_Ascending};
                std::sort(snapshots.begin(), snapshots.end(),
                          [selectedColumn, bAscendingRequired](const ig::AssetManager::Snapshot& lhs, const ig::AssetManager::Snapshot& rhs)
                          {
                              int comp{};
                              switch (selectedColumn)
                              {
                              case 0:
                                  return bAscendingRequired ? lhs.Info.GetCategory() < rhs.Info.GetCategory() : lhs.Info.GetCategory() > rhs.Info.GetCategory();

                              case 1:
                                  return bAscendingRequired ? lhs.Info.GetGuid().bytes() < rhs.Info.GetGuid().bytes() : lhs.Info.GetGuid().bytes() > rhs.Info.GetGuid().bytes();
                              case 2:
                                  comp = lhs.Info.GetVirtualPath().Compare(rhs.Info.GetVirtualPath());
                                  return bAscendingRequired ? comp < 0 : comp > 0;

                              case 3:
                                  return bAscendingRequired ? lhs.Info.GetScope() < rhs.Info.GetScope() : lhs.Info.GetScope() > rhs.Info.GetScope();
                              case 4:
                                  return bAscendingRequired ? lhs.RefCount < rhs.RefCount : lhs.RefCount > rhs.RefCount;

                              default:
                                  IG_CHECK_NO_ENTRY();
                                  return false;
                              };
                          });

                const ig::Guid lastSelectedGuid{mainTableSelectedIdx != -1 ? snapshots[mainTableSelectedIdx].Info.GetGuid() : ig::Guid{}};
                mainTableSelectedIdx = -1;
                if (lastSelectedGuid.isValid())
                {
                    const auto foundItr = std::find_if(snapshots.cbegin(), snapshots.cend(),
                                                       [lastSelectedGuid](const ig::AssetManager::Snapshot& snapshot)
                                                       { return snapshot.Info.GetGuid() == lastSelectedGuid; });

                    mainTableSelectedIdx = foundItr != snapshots.cend() ? static_cast<int>(foundItr - snapshots.cbegin()) : -1;
                }

                sortSpecs->SpecsDirty = false;
                bIsDirty = false;
            }

            // Type // GUID // VIRTUAL PATH // Scope // REF COUNT
            for (int idx = 0; idx < snapshots.size(); ++idx)
            {
                const ig::AssetManager::Snapshot& snapshot{snapshots[idx]};
                const ig::EAssetCategory assetCategory{snapshot.Info.GetCategory()};
                if (assetCategoryFilter == ig::EAssetCategory::Unknown || assetCategoryFilter == assetCategory)
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text(ToCStr(snapshot.Info.GetCategory()));
                    ImGui::TableNextColumn();
                    if (ImGui::Selectable(std::format("{}", snapshot.Info.GetGuid()).c_str(), (selectedIdx == idx),
                                          ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap))
                    {
                        selectedIdx = idx;
                        if (bSelectionDirtyFlagPtr != nullptr)
                        {
                            *bSelectionDirtyFlagPtr = true;
                        }
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

    void AssetInspector::RenderInspector()
    {
        ig::ImGuiX::SeparatorText("Inspector");
        if (mainTableSelectedIdx != -1)
        {
            ig::AssetManager::Snapshot selectedSnapshot{snapshots[mainTableSelectedIdx]};
            const ig::AssetInfo& assetInfo{selectedSnapshot.Info};
            RenderAssetInfo(assetInfo);
            RenderEdit(assetInfo);
            RenderPreview(assetInfo);
        }
    }

    void AssetInspector::RenderEdit(const ig::AssetInfo& assetInfo)
    {
        if (assetInfo.GetScope() != ig::EAssetScope::Engine)
        {
            switch (assetInfo.GetCategory())
            {
            case ig::EAssetCategory::Material:
                RenderMaterialEdit(assetInfo);
                break;
            case ig::EAssetCategory::StaticMesh:
                RenderStaticMeshEdit(assetInfo);
                break;
            default:
                break;
            }
        }
    }

    void AssetInspector::RenderPreview(const ig::AssetInfo& assetInfo)
    {
        ig::ImGuiX::SeparatorText("Preview");
        switch (assetInfo.GetCategory())
        {
        case ig::EAssetCategory::Texture:
            RenderTexturePreview(assetInfo);
            break;
        default:
            break;
        }
    }

    void AssetInspector::RenderTexturePreview(const ig::AssetInfo& assetInfo)
    {
        ig::RenderContext& renderContext{ig::Engine::GetRenderContext()};
        ig::AssetManager& assetManager{ig::Engine::GetAssetManager()};

        if (bIsMainSelectionDirty)
        {
            bIsMainSelectionDirty = false;
            if (previewTexture)
            {
                assetManager.Unload(previewTexture);
            }

            previewTexture = assetManager.Load<ig::Texture>(assetInfo.GetGuid());
        }
        IG_CHECK(previewTexture);

        ig::Texture* previewTexturePtr = assetManager.Lookup(previewTexture);
        IG_CHECK(previewTexturePtr != nullptr);

        const ig::Handle<ig::GpuView> previewTextureSrv = previewTexturePtr->GetShaderResourceView();
        IG_CHECK(previewTextureSrv);
        ig::GpuView* previewTextureSrvPtr = renderContext.Lookup(previewTextureSrv);
        IG_CHECK(previewTextureSrvPtr != nullptr);
        IG_CHECK(previewTextureSrvPtr->HasValidGpuHandle());
        ImGui::Image(previewTextureSrvPtr->GpuHandle.ptr, ImVec2{256.f, 256.f});
    }

    void AssetInspector::RenderMaterialEdit(const ig::AssetInfo& assetInfo)
    {
        ig::AssetManager& assetManager{ig::Engine::GetAssetManager()};
        std::optional<ig::Material::LoadDesc> loadDescOpt{assetManager.GetLoadDesc<ig::Material>(assetInfo.GetGuid())};
        ig::ImGuiX::SeparatorText("Material");
        if (!loadDescOpt)
        {
            ImGui::Text("Invalid Material!");
            return;
        }

        ig::Material::LoadDesc loadDesc{*loadDescOpt};
        RenderSelector("Diffuse", loadDesc.DiffuseTexGuid);
        int newDiffuseIdx{RenderSelectorPopup(ig::EAssetCategory::Texture)};
        if (newDiffuseIdx != -1)
        {
            const ig::AssetInfo& newDiffuseInfo{snapshots[newDiffuseIdx].Info};
            IG_CHECK(newDiffuseInfo.IsValid());
            IG_CHECK(newDiffuseInfo.GetCategory() == ig::EAssetCategory::Texture);
            loadDesc.DiffuseTexGuid = newDiffuseInfo.GetGuid();
            assetManager.UpdateLoadDesc<ig::Material>(assetInfo.GetGuid(), loadDesc);
            assetManager.Reload<ig::Material>(assetInfo.GetGuid());
        }
    }

    void AssetInspector::RenderStaticMeshEdit(const ig::AssetInfo& assetInfo)
    {
        ig::AssetManager& assetManager{ig::Engine::GetAssetManager()};
        std::optional<ig::StaticMesh::LoadDesc> loadDescOpt{assetManager.GetLoadDesc<ig::StaticMesh>(assetInfo.GetGuid())};
        ig::ImGuiX::SeparatorText("Static Mesh");
        if (!loadDescOpt)
        {
            ImGui::Text("Invalid Static Mesh!");
            return;
        }
        ImGui::Text("Num Vertices: %u", loadDescOpt->NumVertices);

        for (ig::U8 lod = 0; lod < loadDescOpt->NumLods; ++lod)
        {
            ImGui::Text("LOD%u Num Indices: %u", (ig::U32)lod, loadDescOpt->NumIndicesPerLod[lod]);
        }

        ig::AABB aabb = loadDescOpt->AABB;
        ImGui::Text("AABB(Min) ");
        ImGui::SameLine();
        ig::ImGuiX::EditVector3("AABB(Min)", aabb.Min, 0.f, "%.4f", true);
        ImGui::Text("AABB(Max) ");
        ImGui::SameLine();
        ig::ImGuiX::EditVector3("AABB(Max)", aabb.Max, 0.f, "%.4f", true);
    }

    void AssetInspector::RenderAssetInfo(const ig::AssetInfo& assetInfo)
    {
        ImGui::TextDisabled(std::format("Category: {}", assetInfo.GetCategory()).c_str());
        ImGui::TextDisabled(std::format("GUID: {}", assetInfo.GetGuid()).c_str());
        ImGui::TextDisabled(std::format("Virtual Path: {}", assetInfo.GetVirtualPath()).c_str());
        ImGui::TextDisabled(std::format("Scope: {}", assetInfo.GetScope()).c_str());
    }

    void AssetInspector::RenderSelector(const char* label, const ig::Guid guid)
    {
        IG_CHECK(label != nullptr);
        if (!ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth))
        {
            return;
        }

        ig::AssetManager& assetManager{ig::Engine::GetAssetManager()};
        std::optional<ig::AssetInfo> assetInfoOpt{assetManager.GetAssetInfo(guid)};
        if (!assetInfoOpt)
        {
            return;
        }

        const ig::AssetInfo& assetInfo{*assetInfoOpt};
        RenderAssetInfo(assetInfo);
        if (!ImGui::Button("Select"))
        {
            return;
        }

        bOpenSelectorPopup = true;
    }

    int AssetInspector::RenderSelectorPopup(const ig::EAssetCategory selectAssetType)
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
            if (ImGui::Button("Select", ImVec2{100.f, 30.f}) || ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                selectedIdx = selectorTableSelectedIdx;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Close", ImVec2{80.f, 30.f}))
            {
                ImGui::CloseCurrentPopup();
            }
            RenderAssetTable(selectAssetType, selectorTableSelectedIdx);
            ImGui::EndPopup();
        }

        return selectedIdx;
    }
} // namespace fe
