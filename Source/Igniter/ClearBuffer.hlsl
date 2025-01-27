struct ClearParams
{
    uint TargetBufferUav;
};

ConstantBuffer<ClearParams> gClearParams;

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    RWByteAddressBuffer buffer = ResourceDescriptorHeap[gClearParams.TargetBufferUav];
    buffer.Store(DTid.x, 0);
}