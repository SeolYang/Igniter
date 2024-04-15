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
        StaticMesh& staticMesh{*staticMeshComponent.Mesh};

        const StaticMesh::Desc& staticMeshSnapshot{staticMesh.GetSnapshot()};
        ImGui::Text(std::format("{}", staticMeshSnapshot.Info).c_str());

        Material&             material{*staticMesh.GetMaterial()};
        const Material::Desc& materialSnapshot{material.GetSnapshot()};
        ImGui::Text(std::format("{}", materialSnapshot.Info).c_str());
    }
} // namespace ig
