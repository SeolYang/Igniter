#pragma once

namespace ig
{
    enum class ELightType : U32
    {
        Point,
    };

    constexpr U16 kMaxNumLights = NumericMaxOfValue(kMaxNumLights);
    struct Light
    {
        ELightType Type = ELightType::Point;
        float Radius = 1.f;
    };
} // namespace ig
