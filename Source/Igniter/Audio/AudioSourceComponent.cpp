#include "Igniter/Igniter.h"
#include "Igniter/ImGui/ImGuiExtensions.h"
#include "Igniter/Audio/AudioSourceComponent.h"

namespace ig
{
    template <>
    void DefineMeta<AudioSourceComponent>()
    {
        IG_META_SET_ON_INSPECTOR(AudioSourceComponent);
    }
    IG_META_DEFINE_AS_COMPONENT(AudioSourceComponent);

    template <>
    void OnInspector<AudioSourceComponent>(Registry* registry, const Entity entity)
    {
        IG_CHECK(registry != nullptr && entity != entt::null);
        AudioSourceComponent& audioSource = registry->get<AudioSourceComponent>(entity);

        /* @todo Audio Asset 선택 가능하도록 하기 */
        ImGui::SliderFloat("Volume", &audioSource.Volume, 0.f, 1.f);
        ImGui::SliderFloat("Pitch", &audioSource.Pitch, 0.f, 1.f);
        ImGui::SliderFloat("Pan", &audioSource.Pan, -1.f, 1.f);
        ImGui::Checkbox("Mute", &audioSource.bMute);
        ImGui::Checkbox("Loop", &audioSource.bLoop);
        if (ImGui::InputFloat("Min Distance", &audioSource.MinDistance, 0.1f, 10.f, "%.3f"))
        {
            audioSource.MinDistance = std::max(0.f, audioSource.MinDistance);
        }
        if (ImGui::InputFloat("Max Distance", &audioSource.MaxDistance, 0.1f, 10.f, "%.3f"))
        {
            audioSource.MaxDistance = std::max(0.f, audioSource.MaxDistance);
        }

        /* Read Only */
        ImGui::Text(std::format("Latest Status: {}", audioSource.LatestStatus).c_str());
        if (ImGuiX::BeginEnumCombo("Next Event", audioSource.NextEvent))
        {
            ImGuiX::EndEnumCombo();
        }

        if (ImGui::Button("Update Properties"))
        {
            audioSource.bShouldUpdatePropertiesOnThisFrame = true;
        }
    }
}
