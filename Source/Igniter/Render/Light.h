#pragma once

namespace ig
{
    enum class ELightType : U32
    {
        Point,
    };

    constexpr U32 kMaxNumLights = 32; //(std::numeric_limits<U16>::max() + 1)/4;
    struct Light
    {
        ELightType Type = ELightType::Point;
        float Radius = 1.f;
    };
} // namespace ig
