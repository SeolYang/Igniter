#include <FpsCameraController.h>

IG_DEFINE_COMPONENT(FpsCameraController);

namespace ig
{
    template <>
    void OnImGui<FpsCameraController>(Registry&, const Entity)
    {
    }
} // namespace ig