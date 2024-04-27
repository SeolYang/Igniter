#include <Igniter.h>
#include <Core/Engine.h>
#include <Core/Json.h>
#include <Asset/AssetManager.h>
#include <Component/StaticMeshComponent.h>

namespace ig
{
    template <>
    void DefineMeta<StaticMeshComponent>()
    {
        IG_SET_META_ON_INSPECTOR_FUNC(StaticMeshComponent, StaticMeshComponent::OnInspector);
        IG_SET_META_JSON_SERIALIZABLE_COMPONENT(StaticMeshComponent);
    }

    StaticMeshComponent::~StaticMeshComponent()
    {
        AssetManager& assetManager = Igniter::GetAssetManager();
        assetManager.Unload(Mesh);
    }

    Json& StaticMeshComponent::Serialize(Json& archive) const
    {
        AssetManager& assetManager = Igniter::GetAssetManager();
        StaticMesh* mesh = assetManager.Lookup(Mesh);
        if (mesh != nullptr)
        {
            IG_SERIALIZE_GUID_JSON(archive, mesh->GetSnapshot().Info.GetGuid(), ContainerKey, MeshGuidKey);
        }

        return archive;
    }

    const Json& StaticMeshComponent::Deserialize(const Json& archive)
    {
        Guid staticMeshGuid{};
        IG_DESERIALIZE_GUID_JSON(archive, staticMeshGuid, ContainerKey, MeshGuidKey, Guid{});
        if (staticMeshGuid.isValid())
        {
            AssetManager& assetManager = Igniter::GetAssetManager();
            Mesh = assetManager.Load<StaticMesh>(staticMeshGuid);
        }

        return archive;
    }

    void StaticMeshComponent::OnInspector(Registry* registry, const Entity entity)
    {
        IG_CHECK(registry != nullptr && entity != entt::null);
        const AssetManager& assetManager = Igniter::GetAssetManager();
        StaticMeshComponent& staticMeshComponent = registry->get<StaticMeshComponent>(entity);
        IG_CHECK(staticMeshComponent.Mesh);
        const StaticMesh* staticMeshPtr = assetManager.Lookup(staticMeshComponent.Mesh);

        const StaticMesh::Desc& staticMeshSnapshot{staticMeshPtr->GetSnapshot()};
        ImGui::Text(std::format("{}", staticMeshSnapshot.Info).c_str());

        const Material* materialPtr = assetManager.Lookup(staticMeshPtr->GetMaterial());
        const Material::Desc& materialSnapshot{materialPtr->GetSnapshot()};
        ImGui::Text(std::format("{}", materialSnapshot.Info).c_str());
    }

    IG_DEFINE_TYPE_META_AS_COMPONENT(StaticMeshComponent);
}    // namespace ig
