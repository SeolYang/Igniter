#pragma once
#include <Igniter.h>
#include <Asset/AssetManager.h>
#include <ImGui/ImGuiLayer.h>

namespace ig
{
    class AssetSnapshotPanel : public ImGuiLayer
    {
    public:
        AssetSnapshotPanel();
        ~AssetSnapshotPanel() = default;

        void Render() override;

    private:
        Mutex mutex{};
        std::vector<AssetManager::Snapshot> snapshots{};
        bool bDirty{ true };
        chrono::system_clock::time_point lastUpdated{ chrono::system_clock::now() };

        std::optional<EAssetType> selectedTypeFilter{};
    };
} // namespace ig