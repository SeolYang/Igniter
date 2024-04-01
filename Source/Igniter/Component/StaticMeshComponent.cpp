#include <Igniter.h>
#include <Core/Engine.h>
#include <Asset/StaticMesh.h>
#include <Component/StaticMeshComponent.h>

namespace ig
{
    IG_DEFINE_COMPONENT(StaticMeshComponent);

    template <>
    void OnImGui<StaticMeshComponent>(Registry& registry, const Entity entity)
    {
        StaticMeshComponent& staticMeshComponent = registry.get<StaticMeshComponent>(entity);
        IG_CHECK(staticMeshComponent.Mesh);
        StaticMesh& staticMesh = *staticMeshComponent.Mesh;

        const StaticMesh::Desc snapshot = staticMesh.GetSnapshot();
        const AssetInfo& assetInfo = snapshot.Info;
        ImGui::Text(std::format("{}({})", assetInfo.VirtualPath.ToStringView(), assetInfo.Guid.str()).c_str());
    }
} // namespace ig