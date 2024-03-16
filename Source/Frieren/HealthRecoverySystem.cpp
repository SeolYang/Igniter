#include <HealthRecoverySystem.h>
#include <Core/Igniter.h>
#include <Core/Timer.h>
#include <Core/Log.h>
#include <HealthComponent.h>
#include <HealthRecoveryBuff.h>

void HealthRecoverySystem::Update(ig::Registry& registry)
{
    const ig::Timer& timer = ig::Igniter::GetTimer();

    const auto recoveryTargetView = registry.view<HealthComponent, HealthRecoveryBuff>();
    for (auto [entity, health, healthRecoveryBuff] : recoveryTargetView.each())
    {
        const float recoveredHealth = health.value + healthRecoveryBuff.recoveryAmout;

        if (healthRecoveryBuff.elapsedTime >= healthRecoveryBuff.period)
        {
            IG_LOG(ig::LogInfo, "HP recovered from {} to {}", health.value, recoveredHealth);
            healthRecoveryBuff.elapsedTime = 0.f;
            health.value = std::min(recoveredHealth, HealthComponent::Maximum);
        }
        else
        {
            healthRecoveryBuff.elapsedTime += timer.GetDeltaTime();
        }

        if (health.value >= HealthComponent::Maximum)
        {
            IG_LOG(ig::LogInfo, "HP fully recovered!");
            registry.erase<HealthRecoveryBuff>(entity);
        }
    }
}