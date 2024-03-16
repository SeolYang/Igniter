#pragma once
#include <Gameplay/Common.h>

namespace fe
{
    class Timer;
} // namespace fe

class RotateEnemySystem
{
public:
    RotateEnemySystem(const float rotateDegsPerSec = 2.f);

    void Update(fe::Registry& registry);

private:
    const fe::Timer& timer;
    const float rotateDegsPerSec;
};