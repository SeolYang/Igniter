float LinearizeDepth(float d, float zNearPlane, float zFarPlane)
{
    return -(zFarPlane * zNearPlane) / (d * (zFarPlane - zNearPlane) - zFarPlane);
}

float LinearizeDepthReverseZ(float d, float zNearPlane, float zFarPlane)
{
    return LinearizeDepth(d, zFarPlane, zNearPlane);
}
