#include "Igniter/Igniter.h"
#include "Igniter/ImGui/AssetSelectModalPopup.h"
#include "Igniter/Core/Engine.h"

namespace ig::ImGuiX
{
    AssetSelectModalPopup::AssetSelectModalPopup(const ig::String label, const ig::EAssetCategory assetCategoryToFilter)
        : label(label)
        , assetInfoChildLabel(std::format("{}##Child", label))
        , assetCategoryToFilter(assetCategoryToFilter)
    {
        IG_CHECK(assetCategoryToFilter != ig::EAssetCategory::Unknown);
    }

    void AssetSelectModalPopup::Open()
    {
        selectedGuid = {};
        bAssetSelected = false;
        TakeAssetSnapshotsFromManager();
        ImGui::OpenPopup(label.ToCString());
    }

    void AssetSelectModalPopup::TakeAssetSnapshotsFromManager()
    {
        cachedAssetInfos.clear();
        Vector<AssetSnapshot> snapshots = ig::Engine::GetAssetManager().TakeSnapshots();
        if (snapshots.empty())
        {
            return;
        }

        cachedAssetInfos.reserve(snapshots.size());
        for (const AssetSnapshot& snapshot : snapshots)
        {
            if (snapshot.Info.GetCategory() == assetCategoryToFilter)
            {
                cachedAssetInfos.emplace_back(AssetInfo{snapshot, ig::String{std::format("{}##Selectable", snapshot.Info.GetVirtualPath())}});
            }
        }

        std::sort(cachedAssetInfos.begin(), cachedAssetInfos.end(), [](const AssetInfo& lhs, const AssetInfo& rhs)
                  { return (lhs.Snapshot.Info.GetVirtualPath().Compare(rhs.Snapshot.Info.GetVirtualPath())) < 0; });
    }

    bool AssetSelectModalPopup::Begin()
    {
        if (!ImGui::BeginPopupModal(label.ToCString(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            return false;
        }

        filter.Draw();
        ImGui::Separator();

        if (ImGui::BeginChild(assetInfoChildLabel.ToCString(),
                              ImVec2{-FLT_MIN, ImGui::GetTextLineHeightWithSpacing() * 8.f},
                              ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeY,
                              ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar))
        {
            for (const AssetInfo& cachedAssetInfo : cachedAssetInfos)
            {
                const ig::Guid& guid = cachedAssetInfo.Snapshot.Info.GetGuid();
                bool bSelected = guid == selectedGuid;
                if (filter.PassFilter(cachedAssetInfo.Snapshot.Info.GetVirtualPath().ToCString()) &&
                    ImGui::Selectable(cachedAssetInfo.SelectableLabel.ToCString(), &bSelected, ImGuiSelectableFlags_DontClosePopups))
                {
                    selectedGuid = guid;
                }
            }
        }
        ImGui::EndChild();

        const bool bShouldActiveSelectButton = selectedGuid.isValid();
        const F32 buttonWidth = bShouldActiveSelectButton ? ImGui::GetContentRegionAvail().x * 0.49f : ImGui::GetContentRegionAvail().x;
        if (bShouldActiveSelectButton && ImGui::Button("Select", ImVec2{buttonWidth, 0.f}))
        {
            bAssetSelected = true;
            ImGui::CloseCurrentPopup();
        }

        if (bShouldActiveSelectButton)
        {
            ImGui::SameLine();
        }

        if (ImGui::Button("Close", ImVec2{buttonWidth, 0.f}))
        {
            bAssetSelected = false;
            ImGui::CloseCurrentPopup();
        }
        return true;
    }

    void AssetSelectModalPopup::End()
    {
        ImGui::EndPopup();
    }
} // namespace ig::ImGuiX
