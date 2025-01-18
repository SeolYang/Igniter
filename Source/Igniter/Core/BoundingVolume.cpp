#include "Igniter/Igniter.h"
#include "Igniter/Core/Json.h"
#include "Igniter/Core/BoundingVolume.h"

namespace ig
{
    Json& AxisAlignedBoundingBox::Serialize(Json& archive) const
    {
        IG_SERIALIZE_TO_JSON(AxisAlignedBoundingBox, archive, Min.x);
        IG_SERIALIZE_TO_JSON(AxisAlignedBoundingBox, archive, Min.y);
        IG_SERIALIZE_TO_JSON(AxisAlignedBoundingBox, archive, Min.z);
        IG_SERIALIZE_TO_JSON(AxisAlignedBoundingBox, archive, Max.x);
        IG_SERIALIZE_TO_JSON(AxisAlignedBoundingBox, archive, Max.y);
        IG_SERIALIZE_TO_JSON(AxisAlignedBoundingBox, archive, Max.z);
        return archive;
    }

    const Json& AxisAlignedBoundingBox::Deserialize(const Json& archive)
    {
        IG_DESERIALIZE_FROM_JSON(AxisAlignedBoundingBox, archive, Min.x);
        IG_DESERIALIZE_FROM_JSON(AxisAlignedBoundingBox, archive, Min.y);
        IG_DESERIALIZE_FROM_JSON(AxisAlignedBoundingBox, archive, Min.z);
        IG_DESERIALIZE_FROM_JSON(AxisAlignedBoundingBox, archive, Max.x);
        IG_DESERIALIZE_FROM_JSON(AxisAlignedBoundingBox, archive, Max.y);
        IG_DESERIALIZE_FROM_JSON(AxisAlignedBoundingBox, archive, Max.z);
        return archive;
    }

    Json& BoundingSphere::Serialize(Json& archive) const
    {
        IG_SERIALIZE_TO_JSON(BoundingSphere, archive, Radius);
        IG_SERIALIZE_TO_JSON(BoundingSphere, archive, Centroid.x);
        IG_SERIALIZE_TO_JSON(BoundingSphere, archive, Centroid.y);
        IG_SERIALIZE_TO_JSON(BoundingSphere, archive, Centroid.z);
        return archive;
    }

    const Json& BoundingSphere::Deserialize(const Json& archive)
    {
        IG_DESERIALIZE_FROM_JSON(BoundingSphere, archive, Radius);
        IG_DESERIALIZE_FROM_JSON(BoundingSphere, archive, Centroid.x);
        IG_DESERIALIZE_FROM_JSON(BoundingSphere, archive, Centroid.y);
        IG_DESERIALIZE_FROM_JSON(BoundingSphere, archive, Centroid.z);
        return archive;
    }
} // namespace ig
