#pragma once
#include "Igniter/Igniter.h"

namespace ig
{
    struct AxisAlignedBoundingBox
    {
    public:
        Vector3 Min{};
        Vector3 Max{};
    };

    struct BoundingSphere
    {
    public:
        Vector3 Centroid{};
        float Radius = 0.f;
    };

    inline BoundingSphere AxisAlignedBoundingBoxToSphere(const AxisAlignedBoundingBox& aabb) noexcept
    {
        return BoundingSphere{(aabb.Min + aabb.Max) * 0.5f, Vector3::Distance(aabb.Min, aabb.Max) * 0.5f};
    }
} // namespace ig
