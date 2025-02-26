struct GenerateDepthPyramidConstants
{
    uint PrevMipUav;
    uint CurrMipUav;

    uint PrevMipWidth;
    uint PrevMipHeight;
    uint CurrMipWidth;
    uint CurrMipHeight;
};

ConstantBuffer<GenerateDepthPyramidConstants> gConstants;

bool IsInsidePrevMipBounds(uint2 offset)
{
    return (offset.x < gConstants.PrevMipWidth && offset.y < gConstants.PrevMipHeight);
}

/* 한 개의 스레드는 항상 하나의 Quad(2x2)를 reduce 한다. */
[NumThreads(8, 8, 1)]
void main(uint2 texCoords : SV_DispatchThreadID)
{
    RWTexture2D<float> prevMipTex = ResourceDescriptorHeap[gConstants.PrevMipUav];
    RWTexture2D<float> currMipTex = ResourceDescriptorHeap[gConstants.CurrMipUav];

    /* Dispatch Thread ID == Reduce될 타겟 밉 레벨의 텍스처 좌표 */
    if (texCoords.x >= gConstants.CurrMipWidth || texCoords.y >= gConstants.CurrMipHeight)
    {
        return;
    }

    uint2 quadOffset = texCoords * 2;
    uint2 depthOffset0 = quadOffset + uint2(0, 0);
    uint2 depthOffset1 = quadOffset + uint2(1, 0);
    uint2 depthOffset2 = quadOffset + uint2(0, 1);
    uint2 depthOffset3 = quadOffset + uint2(1, 1);
    /* Inverted Z-Buffer를 기준으로 가장 먼 물체의 Depth는 0, 가까운 값을 찾아내기 위해선 Max Operation이 필요하다 */
    float depth0 = IsInsidePrevMipBounds(depthOffset0) ? prevMipTex.Load(depthOffset0).x : 0.f;
    float depth1 = IsInsidePrevMipBounds(depthOffset1) ? prevMipTex.Load(depthOffset1).x : 0.f;
    float depth2 = IsInsidePrevMipBounds(depthOffset2) ? prevMipTex.Load(depthOffset2).x : 0.f;
    float depth3 = IsInsidePrevMipBounds(depthOffset3) ? prevMipTex.Load(depthOffset3).x : 0.f;

    float maxDepth = max(max(max(depth0, depth1), depth2), depth3);
    currMipTex[texCoords] = maxDepth;
}
