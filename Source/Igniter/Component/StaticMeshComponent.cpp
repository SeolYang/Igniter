#include "Igniter/Igniter.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Core/Json.h"
#include "Igniter/Asset/AssetManager.h"
#include "Igniter/ImGui/AssetSelectModalPopup.h"
#include "Igniter/Component/StaticMeshComponent.h"

namespace ig
{
    template <>
    void DefineMeta<StaticMeshComponent>()
    {
        IG_SET_ON_INSPECTOR_META(StaticMeshComponent);
        IG_SET_META_JSON_SERIALIZABLE_COMPONENT(StaticMeshComponent);
    }
    IG_DEFINE_COMPONENT_META(StaticMeshComponent);

    template <>
    Json& Serialize<Json, StaticMeshComponent>(Json& archive, const StaticMeshComponent& staticMesh)
    {
        AssetManager& assetManager = Engine::GetAssetManager();
        if (const StaticMesh* mesh = assetManager.Lookup(staticMesh.Mesh);
            mesh != nullptr)
        {
            const Guid& meshGuid = mesh->GetSnapshot().Info.GetGuid();
            IG_SERIALIZE_TO_JSON(StaticMeshComponent, archive, meshGuid);
        }

        return archive;
    }

    template <>
    const Json& Deserialize<Json, StaticMeshComponent>(const Json& archive, StaticMeshComponent& staticMesh)
    {
        /* @todo 실패 시 Engine Default Model의 GUID를 사용하도록 할 것(error model) */
        Guid meshGuid{};
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshComponent, archive, meshGuid);

        if (meshGuid.isValid())
        {
            AssetManager& assetManager = Engine::GetAssetManager();
            if (staticMesh.Mesh)
            {
                assetManager.Unload(staticMesh.Mesh);
            }
            staticMesh.Mesh = assetManager.Load<StaticMesh>(meshGuid);
        }
        
        return archive;
    }

    template <>
    void OnInspector<StaticMeshComponent>(Registry* registry, const Entity entity)
    {
        IG_CHECK(registry != nullptr && entity != entt::null);
        AssetManager& assetManager = Engine::GetAssetManager();
        StaticMeshComponent& staticMeshComponent = registry->get<StaticMeshComponent>(entity);

        if (staticMeshComponent.Mesh)
        {
            const StaticMesh* staticMeshPtr = assetManager.Lookup(staticMeshComponent.Mesh);

            const StaticMesh::Desc& staticMeshSnapshot{staticMeshPtr->GetSnapshot()};
            ImGui::Text(std::format("{}", staticMeshSnapshot.Info).c_str());
        }
        else
        {
            ImGui::Text("Static Mesh Component does not selected.");
        }

        static ImGuiX::AssetSelectModalPopup staticMeshSelectModalPopup{"Select Static Mesh Asset"_fs, EAssetCategory::StaticMesh};
        if (ImGui::Button("Select Asset##StaticMeshComponentInspector", ImVec2{-FLT_MIN, 0.f}))
        {
            staticMeshSelectModalPopup.Open();
        }
        if (staticMeshSelectModalPopup.Begin())
        {
            if (staticMeshSelectModalPopup.IsAssetSelected())
            {
                const Guid selectedGuid = staticMeshSelectModalPopup.GetSelectedAssetGuid();
                if (!selectedGuid.isValid())
                {
                    return;
                }

                Handle<StaticMesh> selectedAsset = assetManager.Load<StaticMesh>(selectedGuid);
                if (staticMeshComponent.Mesh && (selectedAsset != staticMeshComponent.Mesh))
                {
                    assetManager.Unload(staticMeshComponent.Mesh);
                }
                staticMeshComponent.Mesh = selectedAsset;
            }
            staticMeshSelectModalPopup.End();
        }
    }
} // namespace ig
