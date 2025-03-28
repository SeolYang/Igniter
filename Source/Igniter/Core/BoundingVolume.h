#pragma once
#include "Igniter/Igniter.h"

namespace ig
{
    struct AABB
    {
    public:
        Json& Serialize(Json& archive) const;
        const Json& Deserialize(const Json& archive);

    public:
        Vector3 Min{};
        Vector3 Max{};
    };

    struct BoundingSphere
    {
    public:
        Json& Serialize(Json& archive) const;
        const Json& Deserialize(const Json& archive);

    public:
        Vector3 Centroid{};
        F32 Radius = 0.f;
    };

    inline BoundingSphere ToBoundingSphere(const AABB& aabb) noexcept
    {
        return BoundingSphere{.Centroid = (aabb.Min + aabb.Max) * 0.5f, .Radius = Vector3::Distance(aabb.Min, aabb.Max) * 0.5f};
    }

    struct Frustum
    {
    public:
        Plane Near;
        Plane Far;
        Plane Left;
        Plane Right;
        Plane Top;
        Plane Bottom;
    };
} // namespace ig
