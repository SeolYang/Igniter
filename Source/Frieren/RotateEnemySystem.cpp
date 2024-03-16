#include <RotateEnemySystem.h>
#include <Core/Timer.h>
#include <Component/TransformComponent.h>
#include <EnemyArchetype.h>
#include <Enemy.h>
#include <Core/Igniter.h>

RotateEnemySystem::RotateEnemySystem(const float rotateDegsPerSec)
    : timer(ig::Igniter::GetTimer()),
      rotateDegsPerSec(rotateDegsPerSec)
{
}

void RotateEnemySystem::Update(ig::Registry& registry)
{
    const auto enemyView = registry.view<Enemy, ig::TransformComponent>();
    for (auto [entity, transform] : enemyView.each())
    {
        transform.Rotation = ig::Quaternion::Concatenate(
            ig::Quaternion::CreateFromYawPitchRoll(ig::Vector3{ 0.f, rotateDegsPerSec * timer.GetDeltaTime(), 0.f }),
            transform.Rotation);
    }
}