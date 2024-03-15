#pragma once

namespace fe
{
    class World;
    class Timer;
} // namespace fe

class RotateEnemySystem
{
public:
    RotateEnemySystem(const float rotateDegsPerSec = 2.f);

    void Update(fe::World& world);

private:
    const fe::Timer& timer;
    const float rotateDegsPerSec;
};