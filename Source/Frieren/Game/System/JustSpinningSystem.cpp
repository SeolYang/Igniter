#include "Frieren/Frieren.h"
#include "Igniter/Core/Timer.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Gameplay/World.h"
#include "Igniter/Component/TransformComponent.h"
#include "Frieren/Game/System/JustSpinningSystem.h"

namespace fe
{
    void JustSpinningSystem::Update(const float deltaTime, ig::World& world)
    {
        ig::Registry& registry = world.GetRegistry();
        registry.view<ig::TransformComponent>().each([deltaTime]([[maybe_unused]] const auto entity, ig::TransformComponent& transform)
                                                     { transform.Rotation *= ig::Quaternion::CreateFromYawPitchRoll(deltaTime, 0.f, 0.f); });
    }

    template <>
    void DefineMeta<JustSpinningSystem>() {}

    IG_DEFINE_TYPE_META_AS_GAME_SYSTEM(JustSpinningSystem);
} // namespace fe
