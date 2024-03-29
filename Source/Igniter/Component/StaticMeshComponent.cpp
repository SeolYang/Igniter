#include <Igniter.h>
#include <Core/Engine.h>
#include <Asset/Model.h>
#include <Asset/AssetManager.h>
#include <Component/StaticMeshComponent.h>

namespace ig
{
    IG_DEFINE_COMPONENT(StaticMeshComponent);

    template <>
    void OnImGui<StaticMeshComponent>(Registry& registry, const Entity entity)
    {
        StaticMeshComponent& staticMeshComponent = registry.get<StaticMeshComponent>(entity);
        IG_CHECK(staticMeshComponent.StaticMeshHandle);
        StaticMesh& staticMesh = *staticMeshComponent.StaticMeshHandle;

        AssetManager& assetManager = Igniter::GetAssetManager();
        const AssetInfo info = assetManager.GetAssetInfo(staticMesh.Guid);
        ImGui::Text(std::format("{}({})", info.VirtualPath.ToStringView(), staticMesh.Guid.str()).c_str());
    }
} // namespace ig