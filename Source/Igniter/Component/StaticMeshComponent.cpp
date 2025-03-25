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
        IG_SET_ON_INSPECTOR_META(StaticMeshComponent, StaticMeshComponent::OnInspector);
        IG_SET_META_JSON_SERIALIZABLE_COMPONENT(StaticMeshComponent);
    }

    StaticMeshComponent::StaticMeshComponent(const StaticMeshComponent& other)
    {
        AssetManager& assetManager = Engine::GetAssetManager();
        if (assetManager.Clone(other.Mesh))
        {
            Mesh = other.Mesh;
        }
    }

    StaticMeshComponent::StaticMeshComponent(StaticMeshComponent&& other) noexcept
        : Mesh(std::exchange(other.Mesh, {}))
    {}

    StaticMeshComponent::~StaticMeshComponent()
    {
        Destroy();
    }

    StaticMeshComponent& StaticMeshComponent::operator=(const StaticMeshComponent& rhs)
    {
        this->Destroy();
        AssetManager& assetManager = Engine::GetAssetManager();
        if (assetManager.Clone(rhs.Mesh))
        {
            Mesh = rhs.Mesh;
        }
        return *this;
    }

    StaticMeshComponent& StaticMeshComponent::operator=(StaticMeshComponent&& rhs) noexcept
    {
        this->Destroy();
        Mesh = std::exchange(rhs.Mesh, {});
        return *this;
    }

    Json& StaticMeshComponent::Serialize(Json& archive) const
    {
        AssetManager& assetManager = Engine::GetAssetManager();
        StaticMesh* mesh = assetManager.Lookup(Mesh);
        if (mesh != nullptr)
        {
            archive[kContainerKey][kMeshGuidKey] = ToJson(mesh->GetSnapshot().Info.GetGuid());
        }

        return archive;
    }

    const Json& StaticMeshComponent::Deserialize(const Json& archive)
    {
        Guid staticMeshGuid{};
        if (FromJson(archive[kContainerKey][kMeshGuidKey], staticMeshGuid))
        {
            AssetManager& assetManager = Engine::GetAssetManager();
            Mesh = assetManager.Load<StaticMesh>(staticMeshGuid);
        }

        return archive;
    }

    void StaticMeshComponent::OnInspector(Registry* registry, const Entity entity)
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

    void StaticMeshComponent::Destroy()
    {
        if (Mesh)
        {
            AssetManager& assetManager = Engine::GetAssetManager();
            assetManager.Unload(Mesh);
            Mesh = {};
        }
    }

    IG_DEFINE_COMPONENT_META(StaticMeshComponent);
} // namespace ig
