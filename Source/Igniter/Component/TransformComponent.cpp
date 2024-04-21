#include <Igniter.h>
#include <Component/TransformComponent.h>
#include <Core/Math.h>
#include <ImGui/ImGuiExtensions.h>

namespace ig
{
    IG_DEFINE_COMPONENT(TransformComponent);

    template <>
    void OnImGui<TransformComponent>(Registry& registry, const Entity entity)
    {
        TransformComponent& transform = registry.get<TransformComponent>(entity);
        ImGuiX::EditTransform("Transform", transform);
    }
} // namespace ig
