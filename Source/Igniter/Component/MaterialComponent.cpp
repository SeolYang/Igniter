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
        IG_SET_ON_INSPECTOR_META(MaterialComponent);
        IG_SET_META_JSON_SERIALIZABLE_COMPONENT(MaterialComponent);
    }
    IG_DEFINE_COMPONENT_META(MaterialComponent);

    template <>
    Json& Serialize<Json, MaterialComponent>(Json& archive, const MaterialComponent& materialComponent)
    {
        const Material* material = Engine::GetAssetManager().Lookup(materialComponent.Instance);
        IG_SERIALIZE_TO_JSON_EXPR(MaterialComponent, archive, Instance,
            material != nullptr ?
            material->GetSnapshot().Info.GetGuid() :
            Guid{DefaultMaterialGuid});

        return archive;
    }

    template <>
    const Json& Deserialize<Json, MaterialComponent>(const Json& archive, MaterialComponent& materialComponent)
    {
        AssetManager& assetManager = Engine::GetAssetManager();
        Guid materialGuid{};
        IG_DESERIALIZE_FROM_JSON_TEMP_NO_FALLBACK(MaterialComponent, archive, Instance, materialGuid);
        if (materialGuid.isValid())
        {
            if (materialComponent.Instance)
            {
                assetManager.Unload<Material>(materialComponent.Instance);
            }
            materialComponent.Instance = assetManager.Load<Material>(materialGuid);
        }

        return archive;
    }

    template <>
    void OnInspector<MaterialComponent>(Registry* registry, const Entity entity)
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

                Handle<Material> selectedAsset = assetManager.Load<Material>(selectedGuid);
                if (materialComponent.Instance && (selectedAsset != materialComponent.Instance))
                {
                    assetManager.Unload(materialComponent.Instance);
                }
                materialComponent.Instance = selectedAsset;
            }
            materialSelectPopup.End();
        }
    }
} // namespace ig
