#include "Igniter/Igniter.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Core/Json.h"
#include "Igniter/Asset/AssetManager.h"
#include "Igniter/Asset/Material.h"
#include "Igniter/ImGui/AssetSelectModalPopup.h"
#include "Igniter/Component/MaterialComponent.h"

namespace ig
{
    template <>
    void DefineMeta<MaterialComponent>()
    {
        IG_SET_META_ON_INSPECTOR_FUNC(MaterialComponent, MaterialComponent::OnInspector);
        IG_SET_META_JSON_SERIALIZABLE_COMPONENT(MaterialComponent);
    }

    MaterialComponent::MaterialComponent() :
        Instance(Engine::GetAssetManager().LoadMaterial(Guid{DefaultMaterialGuid}))
    {
    }

    MaterialComponent::MaterialComponent(const MaterialComponent& other) :
        Instance(Engine::GetAssetManager().Clone(other.Instance))
    {
    }

    MaterialComponent::MaterialComponent(MaterialComponent&& other) noexcept :
        Instance(std::exchange(other.Instance, {}))
    {
    }

    MaterialComponent::~MaterialComponent()
    {
        Destroy();
    }

    MaterialComponent& MaterialComponent::operator=(const MaterialComponent& rhs)
    {
        Destroy();
        Instance = Engine::GetAssetManager().Clone(rhs.Instance);
        return *this;
    }

    MaterialComponent& MaterialComponent::operator=(MaterialComponent&& rhs) noexcept
    {
        Destroy();
        Instance = std::exchange(rhs.Instance, {});
        return *this;
    }

    Json& MaterialComponent::Serialize(Json& archive) const
    {
        Material* material = Engine::GetAssetManager().Lookup(Instance);
        if (material != nullptr)
        {
            IG_SERIALIZE_GUID_JSON(archive, material->GetSnapshot().Info.GetGuid(), ContainerKey, MeshGuidKey);
        }
        else
        {
            IG_SERIALIZE_GUID_JSON(archive, Guid{DefaultMaterialGuid}, ContainerKey, MeshGuidKey);
        }
        return archive;
    }

    const Json& MaterialComponent::Deserialize(const Json& archive)
    {
        AssetManager& assetManager = Engine::GetAssetManager();
        Guid materialGuid{};
        IG_DESERIALIZE_GUID_JSON(archive, materialGuid, ContainerKey, MeshGuidKey, Guid{});
        if (!materialGuid.isValid())
        {
            materialGuid = Guid{DefaultMaterialGuid};
        }

        Instance = assetManager.Load<Material>(materialGuid);
        IG_CHECK(Instance);
        return archive;
    }

    void MaterialComponent::OnInspector(Registry* registry, const Entity entity)
    {
        IG_CHECK(registry != nullptr && entity != NullEntity);
        AssetManager& assetManager = Engine::GetAssetManager();
        MaterialComponent& materialComponent = registry->get<MaterialComponent>(entity);

        if (materialComponent.Instance)
        {
            const Material* material = assetManager.Lookup(materialComponent.Instance);
            const Material::Desc& desc{material->GetSnapshot()};
            ImGui::Text(std::format("{}", desc.Info).c_str());
        }
        else
        {
            ImGui::Text("Material Instance does not selected.");
        }

        static ImGuiX::AssetSelectModalPopup materialSelectPopup{"Select Material Asset"_fs, EAssetCategory::Material};
        if (ImGui::Button("Select Asset##MaterialComponentInspector", ImVec2{-FLT_MIN, 0.f}))
        {
            materialSelectPopup.Open();
        }
        if (materialSelectPopup.Begin())
        {
            if (materialSelectPopup.IsAssetSelected())
            {
                const Guid selectedGuid = materialSelectPopup.GetSelectedAssetGuid();
                if (!selectedGuid.isValid())
                {
                    return;
                }

                ManagedAsset<Material> selectedAsset = assetManager.Load<Material>(selectedGuid);
                if (materialComponent.Instance && (selectedAsset != materialComponent.Instance))
                {
                    assetManager.Unload(materialComponent.Instance);
                }
                materialComponent.Instance = selectedAsset;
            }
            materialSelectPopup.End();
        }
    }

    void MaterialComponent::Destroy()
    {
        AssetManager& assetManager = Engine::GetAssetManager();
        assetManager.Unload(Instance);
        Instance = {};
    }

    IG_DEFINE_TYPE_META_AS_COMPONENT(MaterialComponent);
} // namespace ig
