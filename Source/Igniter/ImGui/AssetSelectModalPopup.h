#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Asset/AssetManager.h"

namespace ig::ImGuiX
{
    // 1. 요청한 종류에 맞는 에셋 리스트를 선택 가능한 형태로 출력 할 것.
    // 2. 에셋의 Virtual Path(이름)으로 필터링 가능 할 것
    // 3. 언젠가 가능하다면 Preview도 제공..
    // 4. AssetInspector가 항상 열려있어서 실시간으로 에셋 변화를 감지해야 했던 것과 다르게
    // 단순히, 열린 순간에 로드 가능한 에셋의 리스트만 뽑아내면 된다.
    class AssetSelectModalPopup final
    {
    private:
        using AssetSnapshot = ig::AssetManager::Snapshot;

        struct AssetInfo
        {
            AssetSnapshot Snapshot;
            ig::String    SelectableLabel;
        };

    public:
        AssetSelectModalPopup(const ig::String label, const ig::EAssetCategory assetCategoryToFilter);
        AssetSelectModalPopup(const AssetSelectModalPopup&)     = delete;
        AssetSelectModalPopup(AssetSelectModalPopup&&) noexcept = delete;
        ~AssetSelectModalPopup()                                = default;

        AssetSelectModalPopup& operator=(const AssetSelectModalPopup&)     = delete;
        AssetSelectModalPopup& operator=(AssetSelectModalPopup&&) noexcept = delete;

        void Open();
        bool Begin();
        void End();

        [[nodiscard]] bool IsAssetSelected() const { return bAssetSelected; }
        ig::Guid           GetSelectedAssetGuid() const { return selectedGuid; }

    private:
        void TakeAssetSnapshotsFromManager();

    private:
        ig::String               label;
        ig::String               assetInfoChildLabel;
        ig::EAssetCategory       assetCategoryToFilter = ig::EAssetCategory::Unknown;
        eastl::vector<AssetInfo> cachedAssetInfos;
        ImGuiTextFilter          filter;
        ig::Guid                 selectedGuid;
        bool                     bAssetSelected = false;
    };
} // namespace ig::ImGuiX
