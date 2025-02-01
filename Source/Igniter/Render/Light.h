#pragma once

namespace ig
{
    enum class ELightType : U32
    {
        Point,
        Directional,
        Spot
    };

    /*
     * Lumen: Luminous Flux(Power) => lm
     * @Radiometry Equivalent: Radiometry Flux => Watts(W)
     * @W * 683(lm/W) * (Luminous Efficnecy(%)) = lm <683: 1와트당 최대 발광 효능; ex. 1와트로 최대 683 루멘만큼의 빛을 낼 수 있다>
     *
     * Lux: Illuminance/Luminous Exitance => lx = lm/m^2
     * @Radiometry Equivalent: Irradiance/Radiant Exitance => W/m^2
     *
     * Candela: Luminous Intensity => cd = lm/sr
     * @Radiometry Equivalent: Radiant Intensity => W/sr (sr -> steradians; solid angles)
     *
     * Nit: Luminance => nt = lm/(sr*m^2) = cd/m^2
     * @Radiometry Equivalent: Radiance => W/(sr*m^2)
     */
    enum class ELightIntensityUnit : U32
    {
        Lumen,
        Lux,
        Candela,
        Nit,
    };

    inline ELightIntensityUnit ToLightIntensityUnit(const ELightType type)
    {
        switch (type)
        {
        case ELightType::Directional:
            return ELightIntensityUnit::Lux;
        case ELightType::Point:
        case ELightType::Spot:
        default:
            return ELightIntensityUnit::Lumen;
        }
    }

    struct Light
    {
        ELightType Type = ELightType::Point;

        /*
         * Directional Light: Lux
         * Point/Spot Light: Lumen
         */
        float Intensity = 1.f;
        float FalloffRadius = 1.f;
        Vector3 Color = Vector3{1.f, 1.f, 1.f};
    };
} // namespace ig
