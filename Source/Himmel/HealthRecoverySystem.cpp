#include <HealthRecoverySystem.h>
#include <Engine.h>
#include <Core/Timer.h>
#include <Core/Log.h>
#include <HealthComponent.h>
#include <HealthRecoveryBuff.h>
#include <Gameplay/World.h>

void HealthRecoverySystem::Update(fe::World& world)
{
    const fe::Timer& timer = fe::Engine::GetTimer();
    world.Each<HealthComponent, HealthRecoveryBuff>(
        [&timer, &world](const fe::Entity entity, HealthComponent& health, HealthRecoveryBuff& healthRecoveryBuff)
        {
            const float recoveredHealth = health.value + healthRecoveryBuff.recoveryAmout;

            if (healthRecoveryBuff.elapsedTime >= healthRecoveryBuff.period)
            {
                FE_LOG(fe::LogInfo, "HP recovered from {} to {}", health.value, recoveredHealth);
                healthRecoveryBuff.elapsedTime = 0.f;
                health.value = std::min(recoveredHealth, HealthComponent::Maximum);
            }
            else
            {
                healthRecoveryBuff.elapsedTime += timer.GetDeltaTime();
            }

            if (health.value >= HealthComponent::Maximum)
            {
                FE_LOG(fe::LogInfo, "HP fully recovered!");
                world.Detach<HealthRecoveryBuff>(entity);
            }
        });
}