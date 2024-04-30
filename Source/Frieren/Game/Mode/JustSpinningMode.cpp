#include <Frieren.h>
#include <Core/Timer.h>
#include <Core/Engine.h>
#include <Gameplay/World.h>
#include <Component/TransformComponent.h>
#include <Game/Mode/JustSpinningMode.h>

namespace fe
{
    void JustSpinningMode::Update(ig::World& world)
    {
        const Timer& timer = Igniter::GetTimer();
        Registry& registry = world.GetRegistry();
        registry.view<TransformComponent>().each([&timer]([[maybe_unused]] const auto entity, TransformComponent& transform)
        {
            transform.Rotation *= Quaternion::CreateFromYawPitchRoll(timer.GetDeltaTime(), 0.f, 0.f);
        });
    }

    template <>
    void DefineMeta<JustSpinningMode>()
    {
    }

    IG_DEFINE_TYPE_META_AS_GAME_MODE(JustSpinningMode);
}    // namespace fe