#pragma once
#include "Igniter/Gameplay/GameSystem.h"
#include "Frieren/Game/System/FpsCameraControllSystem.h"
#include "Frieren/Game/System/CameraPossessSystem.h"

namespace ig
{
    class World;
}

namespace fe
{
    class JustSpinningSystem final : public ig::GameSystem
    {
    public:
        JustSpinningSystem() = default;
        JustSpinningSystem(const JustSpinningSystem&) = delete;
        JustSpinningSystem(JustSpinningSystem&&) noexcept = delete;
        ~JustSpinningSystem() override = default;

        JustSpinningSystem& operator=(const JustSpinningSystem&) = delete;
        JustSpinningSystem& operator=(JustSpinningSystem&&) noexcept = delete;

        void Update(const float deltaTime, ig::World& world) override;

    };

    IG_DECLARE_TYPE_META(JustSpinningSystem);
}    // namespace fe
