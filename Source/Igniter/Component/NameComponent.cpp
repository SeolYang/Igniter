#include <Component/NameComponent.h>
#include <ImGui/Common.h>

namespace ig
{
    IG_DEFINE_COMPONENT(NameComponent);

    template <>
    void OnImGui<NameComponent>(Registry& registry, const Entity entity)
    {
        NameComponent& nameComponent = registry.get<NameComponent>(entity);
        ImGui::Text(nameComponent.Name.AsCStr());
    }
} // namespace ig