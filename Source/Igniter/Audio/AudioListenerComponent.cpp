#include "Igniter/Igniter.h"
#include "Igniter/Audio/AudioListenerComponent.h"

#include "Igniter/ImGui/ImGuiExtensions.h"

namespace ig
{
    template <>
    void DefineMeta<AudioListenerComponent>()
    {
        IG_META_SET_ON_INSPECTOR(AudioListenerComponent);
    }
    IG_META_DEFINE_AS_COMPONENT(AudioListenerComponent);

    template <>
    void OnInspector<AudioListenerComponent>(Registry* registry, const Entity entity)
    {
        IG_CHECK(registry != nullptr && entity != NullEntity);
        AudioListenerComponent& audioListener = registry->get<AudioListenerComponent>(entity);
        ImGui::Text("Previous Position");
        ImGuiX::EditVector3("PrevPos_Vec3", audioListener.PrevPosition, 0.f, "%0.3f", true);
    }
}
