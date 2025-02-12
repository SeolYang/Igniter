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
