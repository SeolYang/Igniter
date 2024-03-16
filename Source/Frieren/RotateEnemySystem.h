#pragma once
#include <Gameplay/Common.h>

namespace ig
{
    class Timer;
} // namespace ig

class RotateEnemySystem
{
public:
    RotateEnemySystem(const float rotateDegsPerSec = 2.f);

    void Update(ig::Registry& registry);

private:
    const ig::Timer& timer;
    const float rotateDegsPerSec;
};