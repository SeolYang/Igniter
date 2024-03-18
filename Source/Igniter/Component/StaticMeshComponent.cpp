#include <Asset/Model.h>
#include <Component/StaticMeshComponent.h>
#include <ImGui/Common.h>

namespace ig
{
    IG_DEFINE_COMPONENT(StaticMeshComponent);

    template <>
    void OnImGui<StaticMeshComponent>(Registry& registry, const Entity entity)
    {
        StaticMeshComponent& staticMeshComponent = registry.get<StaticMeshComponent>(entity);
        IG_CHECK(staticMeshComponent.StaticMeshHandle);
        ImGui::Text(staticMeshComponent.StaticMeshHandle->LoadConfig.Name.c_str());
    }
} // namespace ig