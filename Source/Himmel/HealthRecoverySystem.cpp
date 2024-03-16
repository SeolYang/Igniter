#include <HealthRecoverySystem.h>
#include <Engine.h>
#include <Core/Timer.h>
#include <Core/Log.h>
#include <HealthComponent.h>
#include <HealthRecoveryBuff.h>

void HealthRecoverySystem::Update(fe::Registry& registry)
{
    const fe::Timer& timer = fe::Engine::GetTimer();

    const auto recoveryTargetView = registry.view<HealthComponent, HealthRecoveryBuff>();
    for (auto [entity, health, healthRecoveryBuff] : recoveryTargetView.each())
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
            registry.erase<HealthRecoveryBuff>(entity);
        }
    }
}