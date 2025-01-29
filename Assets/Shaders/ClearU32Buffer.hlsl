struct ClearParams
{
    uint TargetBufferUav;
    uint NumElements;
};

ConstantBuffer<ClearParams> gClearParams;

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x >= gClearParams.NumElements)
    {
        return;
    }
    
    RWStructuredBuffer<uint> buffer = ResourceDescriptorHeap[gClearParams.TargetBufferUav];
    buffer[DTid.x] = 0;
}