float LinearizeDepth(float d, float zNearPlane, float zFarPlane)
{
    return -(zFarPlane * zNearPlane) / (d * (zFarPlane - zNearPlane) - zFarPlane);
}

float LinearizeDepthReverseZ(float d, float zNearPlane, float zFarPlane)
{
    return LinearizeDepth(d, zFarPlane, zNearPlane);
}

uint3 DecodeTriangle(uint encodedTriangle)
{
    return uint3(
        encodedTriangle & 0xFF,
        (encodedTriangle >> 8) & 0xFF,
        (encodedTriangle >> 16) & 0xFF);
}

float3 DecodeNormalX8Y8Z8(uint encodedNormal)
{
    const static float kInvFactor = 1.f / 127.5f;
    return float3(
        (float(encodedNormal & 0xFF) * kInvFactor) - 1.f,
        (float((encodedNormal >> 8) & 0xFF) * kInvFactor) - 1.f,
        (float((encodedNormal >> 16) & 0xFF) * kInvFactor) - 1.f);
}

float3 DecodeNormalX10Y10Z10(uint encodedNormal)
{
    const static float kInvFactor = 1.f / 511.5f;
    return float3(
        (float(encodedNormal & 0x3FF) * kInvFactor) - 1.f,
        (float((encodedNormal >> 10) & 0x3FF) * kInvFactor) - 1.f,
        (float((encodedNormal >> 20) & 0x3FF) * kInvFactor) - 1.f);
}

uint BitfieldMask(uint width, uint min)
{
    return (uint(0xFFFFFFFF) >> (32 - width)) << min;
}

float ExtractMaxAbsScale(float4x4 toWorld)
{
    float3 r0 = float3(toWorld._m00, toWorld._m01, toWorld._m02);
    float3 r1 = float3(toWorld._m10, toWorld._m11, toWorld._m12);
    float3 r2 = float3(toWorld._m20, toWorld._m21, toWorld._m22);

    float dx = dot(r0, r0);
    float dy = dot(r1, r1);
    float dz = dot(r2, r2);
    return sqrt(max(dx, max(dy, dz)));
}

BoundingSphere TransformBoundingSphere(BoundingSphere boundingSphere, float4x4 transform)
{
    BoundingSphere transformedBoundingSphere;
    transformedBoundingSphere.Center = mul(float4(boundingSphere.Center, 1.f), transform).xyz;
    transformedBoundingSphere.Radius = boundingSphere.Radius * ExtractMaxAbsScale(transform);
    return transformedBoundingSphere;
}

bool IntersectFrustum(float invAspectRatio, float4 viewFrusumParams, BoundingSphere viewBoundingSphere)
{
    float leftPlaneSignedDist =
        (invAspectRatio * viewFrusumParams.x * viewBoundingSphere.Center.x) + (viewFrusumParams.y * viewBoundingSphere.Center.z);
    float rightPlaneSignedDist =
        (-invAspectRatio * viewFrusumParams.x * viewBoundingSphere.Center.x) + (viewFrusumParams.y * viewBoundingSphere.Center.z);
    float bottomPlaneSignedDist =
        (viewFrusumParams.x * viewBoundingSphere.Center.y) + (viewFrusumParams.y * viewBoundingSphere.Center.z);
    float topPlaneSignedDist =
        (-viewFrusumParams.x * viewBoundingSphere.Center.y) + (viewFrusumParams.y * viewBoundingSphere.Center.z);

    float nearPlaneSignedDist = viewBoundingSphere.Center.z - viewFrusumParams.z;
    float farPlaneSignedDist = -viewBoundingSphere.Center.z + viewFrusumParams.w;

    return nearPlaneSignedDist > -viewBoundingSphere.Radius &&
        farPlaneSignedDist > -viewBoundingSphere.Radius &&
        leftPlaneSignedDist > -viewBoundingSphere.Radius &&
        rightPlaneSignedDist > -viewBoundingSphere.Radius &&
        bottomPlaneSignedDist > -viewBoundingSphere.Radius &&
        topPlaneSignedDist > -viewBoundingSphere.Radius;
}

uint MapScreenCoverageToLodAuto(float screenCoverage)
{
    const static float kTable[MAX_MESH_LEVEL_OF_DETAILS] = {
        0.0008f,    /* Screen Coverage >= 0.08% */
        0.0005f,   /* 0.1% > Screen Coverage >= 0.05% */
        0.00035f,   /* 0.05% > Screen Coverage >= 0.035% */
        0.0003f,  /* 0.035% Screen Coverage >= 0.03% */
        0.00028f,  /* 0.03% > Screen Coverage >= 0.028% */
        0.00025f,  /* 0.028% > Screen Coverage >= 0.025%*/
        0.0002f, /* 0.025% > Screen Coverage >= 0.02%*/
        0.00015f,  /* 0.02% > Screen Coverage >= 0.015% */
    };

    for (uint lod = 0; lod < MAX_MESH_LEVEL_OF_DETAILS; ++lod)
    {
        if (screenCoverage >= kTable[lod])
        {
            return lod;
        }
    }

    return MAX_MESH_LEVEL_OF_DETAILS - 1;
}
