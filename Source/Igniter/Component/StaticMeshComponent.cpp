#include <Igniter.h>
#include <Core/Engine.h>
#include <Asset/StaticMesh.h>
#include <Asset/AssetManager.h>
#include <Component/StaticMeshComponent.h>

namespace ig
{
    IG_DEFINE_COMPONENT(StaticMeshComponent);

    template <>
    void OnImGui<StaticMeshComponent>(Registry& registry, const Entity entity)
    {
        const AssetManager& assetManager = Igniter::GetAssetManager();
        StaticMeshComponent& staticMeshComponent = registry.get<StaticMeshComponent>(entity);
        IG_CHECK(staticMeshComponent.Mesh);
        const StaticMesh* staticMeshPtr = assetManager.Lookup(staticMeshComponent.Mesh);

        const StaticMesh::Desc& staticMeshSnapshot{staticMeshPtr->GetSnapshot()};
        ImGui::Text(std::format("{}", staticMeshSnapshot.Info).c_str());

        const Material* materialPtr = assetManager.Lookup(staticMeshPtr->GetMaterial());
        const Material::Desc& materialSnapshot{materialPtr->GetSnapshot()};
        ImGui::Text(std::format("{}", materialSnapshot.Info).c_str());
    }

    StaticMeshComponent::~StaticMeshComponent() 
    {
        AssetManager& assetManager = Igniter::GetAssetManager();
        assetManager.Unload(Mesh);
    }
}    // namespace ig
