float4 main(uint vertexId : SV_VertexID) : SV_POSITION
{
    const float2 uv = float2((vertexId << 1) & 2, vertexId & 2);
    return float4(uv * float2(2.f, -2.f) + float2(-1.f, 1.f), 0.f, 1.f);
}
