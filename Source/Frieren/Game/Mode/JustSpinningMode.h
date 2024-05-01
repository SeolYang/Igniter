#pragma once
#include "Igniter/Gameplay/GameMode.h"
#include "Frieren/Game/System/FpsCameraControllSystem.h"
#include "Frieren/Game/System/CameraPossessSystem.h"

namespace ig
{
    class World;
}

namespace fe
{
    class JustSpinningMode final : public ig::GameMode
    {
    public:
        JustSpinningMode() = default;
        JustSpinningMode(const JustSpinningMode&) = delete;
        JustSpinningMode(JustSpinningMode&&) noexcept = delete;
        ~JustSpinningMode() override = default;

        JustSpinningMode& operator=(const JustSpinningMode&) = delete;
        JustSpinningMode& operator=(JustSpinningMode&&) noexcept = delete;

        void Update(ig::World& world) override;

    };

    IG_DECLARE_TYPE_META(JustSpinningMode);
}    // namespace fe
