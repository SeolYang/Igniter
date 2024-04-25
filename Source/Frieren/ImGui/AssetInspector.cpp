#include <Frieren.h>
#include <Core/Engine.h>
#include <Core/Timer.h>
#include <Core/FrameManager.h>
#include <D3D12/GpuBuffer.h>
#include <D3D12/GpuTextureDesc.h>
#include <D3D12/GpuTexture.h>
#include <D3D12/GpuView.h>
#include <D3D12/RenderDevice.h>
#include <Render/RenderContext.h>
#include <ImGui/ImGuiExtensions.h>
#include <ImGui/ImGuiRenderer.h>
#include <ImGui/AssetInspector.h>

namespace fe
{
    AssetInspector::AssetInspector()
    {
        AssetManager& assetManager{Igniter::GetAssetManager()};
        AssetManager::ModifiedEvent& modifiedEvent{assetManager.GetModifiedEvent()};
        modifiedEvent.Subscribe("AssetInspector"_fs,
            [this](const std::reference_wrapper<const AssetManager> assetManagerRef)
            {
                RecursiveLock lock{mutex};
                const AssetManager& assetManager{assetManagerRef.get()};

                const Guid lastSelectedGuid{mainTableSelectedIdx != -1 ? snapshots[mainTableSelectedIdx].Info.GetGuid() : Guid{}};
                snapshots = assetManager.TakeSnapshots();
                bDirty = true;
                mainTableSelectedIdx = -1;
                if (lastSelectedGuid.isValid())
                {
                    const auto foundItr = std::find_if(snapshots.cbegin(), snapshots.cend(),
                        [lastSelectedGuid](const AssetManager::Snapshot& snapshot) { return snapshot.Info.GetGuid() == lastSelectedGuid; });

                    mainTableSelectedIdx = foundItr != snapshots.cend() ? static_cast<int>(foundItr - snapshots.cbegin()) : -1;
                }

                lastUpdated = chrono::system_clock::now();
            });

        snapshots = assetManager.TakeSnapshots();
    }

    void AssetInspector::Render()
    {
        if (ImGui::Begin("Asset Inspector", &bIsVisible, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar))
        {
            RenderMenuBar();
            RenderMainFrame();
            ImGui::End();
        }
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
            std::optional<EAssetCategory> selectedFilter = ImGuiX::EnumMenuItems<EAssetCategory>(mainTableAssetFilter, EAssetCategory::Unknown);
            if (ImGui::MenuItem("No Filters") && !selectedFilter)
            {
                mainTableAssetFilter = EAssetCategory::Unknown;
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

        RecursiveLock lock{mutex};
        constexpr float MainTableRatio = 0.6f;
        constexpr float InspectorRatio = 1.f - MainTableRatio;

        const float width{ImGui::GetContentRegionAvail().x};

        ImGui::BeginGroup();
        ImGui::BeginChild("Table", ImVec2{width * MainTableRatio, 0.f}, ImGuiChildFlags_Border);
        ImGuiX::SeparatorText("Assets");
        RenderAssetTable(mainTableAssetFilter, mainTableSelectedIdx, &bIsMainSelectionDirty);
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("Inspector", ImVec2{width * InspectorRatio - ImGuiX::GetFramePadding().x, 0.f}, ImGuiChildFlags_Border,
            ImGuiWindowFlags_HorizontalScrollbar);
        RenderInspector();
        ImGui::EndChild();
        ImGui::EndGroup();
    }

    void AssetInspector::RenderAssetStats()
    {
        ImGui::Text(std::format("Last Update at {:%Y/%m/%d %H:%M:%S}", lastUpdated).data());
        if (mainTableAssetFilter != EAssetCategory::Unknown)
        {
            const auto IsSelected = [selectedTypeFilter = mainTableAssetFilter](const AssetManager::Snapshot& snapshot)
            { return snapshot.Info.GetCategory() == selectedTypeFilter; };

            const auto IsSelectedCached = [selectedTypeFilter = mainTableAssetFilter](const AssetManager::Snapshot& snapshot)
            { return snapshot.Info.GetCategory() == selectedTypeFilter && snapshot.IsCached(); };

            ImGui::Text(
                std::format("#Imported {}: {}\t#Cached {}: {}", mainTableAssetFilter, std::count_if(snapshots.begin(), snapshots.end(), IsSelected),
                    mainTableAssetFilter, std::count_if(snapshots.begin(), snapshots.end(), IsSelectedCached))
                    .c_str());
        }
        else
        {
            const auto IsCached = [](const AssetManager::Snapshot& snapshot) { return snapshot.IsCached(); };

            ImGui::Text(
                std::format("#Imported Assets: {}\t#Cached Assets: {}", snapshots.size(), std::count_if(snapshots.begin(), snapshots.end(), IsCached))
                    .c_str());
        }
    }

    void AssetInspector::RenderAssetTable(const EAssetCategory assetCategoryFilter, int& selectedIdx, bool* bSelectionDirtyFlagPtr)
    {
        constexpr ImGuiTableFlags TableFlags = ImGuiTableFlags_Reorderable | ImGuiTableFlags_Sortable | ImGuiTableFlags_RowBg |
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
            if (sortSpecs->SpecsDirty || bDirty)
            {
                IG_CHECK(sortSpecs->SpecsCount > 0);
                const ImGuiSortDirection sortDirection{sortSpecs->Specs[0].SortDirection};
                const int16_t selectedColumn{sortSpecs->Specs[0].ColumnIndex};
                const bool bAscendingRequired{sortDirection == ImGuiSortDirection_Ascending};
                std::sort(snapshots.begin(), snapshots.end(),
                    [selectedColumn, bAscendingRequired](const AssetManager::Snapshot& lhs, const AssetManager::Snapshot& rhs)
                    {
                        int comp{};
                        switch (selectedColumn)
                        {
                            case 0:
                                return bAscendingRequired ? lhs.Info.GetCategory() < rhs.Info.GetCategory()
                                                          : lhs.Info.GetCategory() > rhs.Info.GetCategory();

                            case 1:
                                return bAscendingRequired ? lhs.Info.GetGuid().bytes() < rhs.Info.GetGuid().bytes()
                                                          : lhs.Info.GetGuid().bytes() > rhs.Info.GetGuid().bytes();
                            case 2:
                                comp = lhs.Info.GetVirtualPath().ToStringView().compare(rhs.Info.GetVirtualPath().ToStringView());
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

                sortSpecs->SpecsDirty = false;
                bDirty = false;
            }

            // Type // GUID // VIRTUAL PATH // Scope // REF COUNT
            for (int idx = 0; idx < snapshots.size(); ++idx)
            {
                const AssetManager::Snapshot& snapshot{snapshots[idx]};
                const EAssetCategory assetCategory{snapshot.Info.GetCategory()};
                if (assetCategoryFilter == EAssetCategory::Unknown || assetCategoryFilter == assetCategory)
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
        ImGuiX::SeparatorText("Inspector");
        if (mainTableSelectedIdx != -1)
        {
            AssetManager::Snapshot selectedSnapshot{snapshots[mainTableSelectedIdx]};
            const AssetInfo& assetInfo{selectedSnapshot.Info};
            RenderAssetInfo(assetInfo);
            RenderEdit(assetInfo);
            RenderPreview(assetInfo);
        }
    }

    void AssetInspector::RenderEdit(const AssetInfo& assetInfo)
    {
        if (assetInfo.GetScope() != EAssetScope::Engine)
        {
            switch (assetInfo.GetCategory())
            {
                case EAssetCategory::Material:
                    RenderMaterialEdit(assetInfo);
                    break;
                case EAssetCategory::StaticMesh:
                    RenderStaticMeshEdit(assetInfo);
                    break;
                default:
                    break;
            }
        }
    }

    void AssetInspector::RenderPreview(const AssetInfo& assetInfo)
    {
        ImGuiX::SeparatorText("Preview");
        switch (assetInfo.GetCategory())
        {
            case EAssetCategory::Texture:
                RenderTexturePreview(assetInfo);
                break;
            default:
                break;
        }
    }

    void AssetInspector::RenderTexturePreview(const AssetInfo& assetInfo)
    {
        ImGuiRenderer& imguiRenderer{Igniter::GetImGuiRenderer()};
        GpuView reservedSrv{imguiRenderer.GetReservedShaderResourceView()};
        if (bIsMainSelectionDirty)
        {
            for (bool& previewSrvUpdateFlag : bIsPreviewSrvUpdated)
            {
                previewSrvUpdateFlag = false;
            }

            bIsMainSelectionDirty = false;
        }

        const FrameManager& frameManager{Igniter::GetFrameManager()};
        const uint8_t localFrameIdx{frameManager.GetLocalFrameIndex()};
        if (!bIsPreviewSrvUpdated[localFrameIdx])
        {
            AssetManager& assetManager{Igniter::GetAssetManager()};
            previewTextures[localFrameIdx] = assetManager.LoadTexture(assetInfo.GetGuid());
            if (previewTextures[localFrameIdx])
            {
                Texture* previewTexturePtr = assetManager.Lookup(previewTextures[localFrameIdx]);
                IG_CHECK(previewTexturePtr != nullptr);
                const Handle<GpuTexture> gpuTexture = previewTexturePtr->GetGpuTexture();
                GpuTexture* gpuTexturePtr = Igniter::GetRenderContext().Lookup(gpuTexture);
                const GpuTextureDesc& gpuTexDesc{gpuTexturePtr->GetDesc()};
                RenderDevice& renderDevice{Igniter::GetRenderContext().GetRenderDevice()};
                /* #sy_todo RenderContext 차원에서 Update...Descriptor 메서드 지원 */
                renderDevice.UpdateShaderResourceView(
                    reservedSrv, *gpuTexturePtr, GpuTextureSrvDesc{D3D12_TEX2D_SRV{.MostDetailedMip = 0, .MipLevels = 1}}, gpuTexDesc.Format);
                bIsPreviewSrvUpdated[localFrameIdx] = true;
            }
        }

        if (bIsPreviewSrvUpdated[localFrameIdx])
        {
            ImGui::Image(reinterpret_cast<ImTextureID>(reservedSrv.GPUHandle.ptr), ImVec2{256, 256});
        }
    }

    void AssetInspector::RenderMaterialEdit(const AssetInfo& assetInfo)
    {
        AssetManager& assetManager{Igniter::GetAssetManager()};
        std::optional<Material::LoadDesc> loadDescOpt{assetManager.GetLoadDesc<Material>(assetInfo.GetGuid())};
        ImGuiX::SeparatorText("Material");
        if (!loadDescOpt)
        {
            ImGui::Text("Invalid Material!");
            return;
        }

        Material::LoadDesc loadDesc{*loadDescOpt};
        RenderSelector("Diffuse", loadDesc.DiffuseTexGuid);
        int newDiffuseIdx{RenderSelectorPopup(EAssetCategory::Texture)};
        if (newDiffuseIdx != -1)
        {
            const AssetInfo& newDiffuseInfo{snapshots[newDiffuseIdx].Info};
            IG_CHECK(newDiffuseInfo.IsValid());
            IG_CHECK(newDiffuseInfo.GetCategory() == EAssetCategory::Texture);
            loadDesc.DiffuseTexGuid = newDiffuseInfo.GetGuid();
            assetManager.UpdateLoadDesc<Material>(assetInfo.GetGuid(), loadDesc);
            assetManager.Reload<Material>(assetInfo.GetGuid());
        }
    }

    void AssetInspector::RenderStaticMeshEdit(const AssetInfo& assetInfo)
    {
        AssetManager& assetManager{Igniter::GetAssetManager()};
        std::optional<StaticMesh::LoadDesc> loadDescOpt{assetManager.GetLoadDesc<StaticMesh>(assetInfo.GetGuid())};
        ImGuiX::SeparatorText("Static Mesh");
        if (!loadDescOpt)
        {
            ImGui::Text("Invalid Static Mesh!");
            return;
        }

        StaticMesh::LoadDesc loadDesc{*loadDescOpt};
        RenderSelector("Material", loadDesc.MaterialGuid);
        int newMaterialIdx{RenderSelectorPopup(EAssetCategory::Material)};
        if (newMaterialIdx != -1)
        {
            const AssetInfo& newMatInfo{snapshots[newMaterialIdx].Info};
            IG_CHECK(newMatInfo.IsValid());
            IG_CHECK(newMatInfo.GetCategory() == EAssetCategory::Material);
            loadDesc.MaterialGuid = newMatInfo.GetGuid();
            assetManager.UpdateLoadDesc<StaticMesh>(assetInfo.GetGuid(), loadDesc);
            assetManager.Reload<StaticMesh>(assetInfo.GetGuid());
        }
    }

    void AssetInspector::RenderAssetInfo(const AssetInfo& assetInfo)
    {
        ImGui::TextDisabled(std::format("Category: {}", assetInfo.GetCategory()).c_str());
        ImGui::TextDisabled(std::format("GUID: {}", assetInfo.GetGuid()).c_str());
        ImGui::TextDisabled(std::format("Virtual Path: {}", assetInfo.GetVirtualPath()).c_str());
        ImGui::TextDisabled(std::format("Scope: {}", assetInfo.GetScope()).c_str());
    }

    void AssetInspector::RenderSelector(const char* label, const Guid guid)
    {
        IG_CHECK(label != nullptr);
        if (!ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth))
        {
            return;
        }

        AssetManager& assetManager{Igniter::GetAssetManager()};
        std::optional<AssetInfo> assetInfoOpt{assetManager.GetAssetInfo(guid)};
        if (!assetInfoOpt)
        {
            return;
        }

        const AssetInfo& assetInfo{*assetInfoOpt};
        RenderAssetInfo(assetInfo);
        if (!ImGui::Button("Select"))
        {
            return;
        }

        bOpenSelectorPopup = true;
    }

    int AssetInspector::RenderSelectorPopup(const EAssetCategory selectAssetType)
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
}    // namespace fe
