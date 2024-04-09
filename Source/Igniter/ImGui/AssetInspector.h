#pragma once
#include <Igniter.h>
#include <Asset/AssetManager.h>
#include <ImGui/ImGuiLayer.h>

namespace ig
{
    class AssetInspector : public ImGuiLayer
    {
    public:
        AssetInspector();
        ~AssetInspector() = default;

        void Render() override;

    private:
        Mutex mutex{};
        std::vector<AssetManager::Snapshot> snapshots{};
        bool bDirty{ true };
        chrono::system_clock::time_point lastUpdated{ chrono::system_clock::now() };

        std::optional<EAssetType> selectedTypeFilter{};
        int selectedIdx{ -1 };
        AssetManager::Snapshot selectedSnapshot{};
    };
} // namespace ig