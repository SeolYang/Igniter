#include <RotateEnemySystem.h>
#include <Core/Timer.h>
#include <Render/TransformComponent.h>
#include <EnemyArchetype.h>
#include <Enemy.h>
#include <Engine.h>

RotateEnemySystem::RotateEnemySystem(const float rotateDegsPerSec)
    : timer(fe::Engine::GetTimer()),
      rotateDegsPerSec(rotateDegsPerSec)
{
}

void RotateEnemySystem::Update(fe::Registry& registry)
{
    const auto enemyView = registry.view<Enemy, fe::TransformComponent>();
    for (auto [entity, transform] : enemyView.each())
    {
        transform.Rotation = fe::Quaternion::Concatenate(
            fe::Quaternion::CreateFromYawPitchRoll(fe::Vector3{ 0.f, rotateDegsPerSec * timer.GetDeltaTime(), 0.f }),
            transform.Rotation);
    }
}