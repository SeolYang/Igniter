#include <Enemy.h>

IG_DEFINE_COMPONENT(Enemy);

namespace ig
{
    template <>
    void OnImGui<Enemy>(Registry&, const Entity)
    {
    }
}