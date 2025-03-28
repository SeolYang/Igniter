#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/String.h"
#include "Igniter/D3D12/GpuBufferDesc.h"
#include "Igniter/D3D12/GpuFence.h"
#include "Igniter/Render/Common.h"

namespace ig
{
    enum class EGpuStorageFlags : U8
    {
        None                   = 0,
        ShaderReadWrite        = 1 << 0,
        EnableUavCounter       = 1 << 1,
        EnableLinearAllocation = 1 << 2,
        RawBuffer              = 1 << 3, /* GPU Buffer 자체를 RawBuffer로 사용. 이 경우 Srv 자체가 ByteAddressBuffer용 Srv */
        CreateRawSrv           = 1 << 4, /* GPU Buffer 자체는 RawBuffer가 아님. 단 RawBuffer로 재해석하는 Srv를 별도로 생성 */
    };

    IG_ENUM_FLAGS(EGpuStorageFlags);

    struct GpuStorageDesc
    {
        std::string_view DebugName = "UnknownStorage";
        U32 ElementSize = 0;
        U32 NumInitElements = 0;
        EGpuStorageFlags Flags = EGpuStorageFlags::None;
    };

    class RenderContext;
    class GpuBuffer;
    class GpuView;
    // 주의: 렌더링 로직 내에서 Allocation/Deallocation을 지양할 것!
    // 해당 저장 공간(버퍼) 대한 작업 시, 반드시 Storage Fence를 사용해서 올바르게
    // 버퍼 접근에 대한 동기화가 올바르게 일어 날 수 있도록 하여야한다.
    //
    // 개선 희망 사항: 만약 할당 정보를 항상 핸들을 사용해서 접근 할 수 있게 만들면 내부적으로 Defragmentation
    // 알고리즘을 구현 할 수도 있을 것 같음.
    class GpuStorage final
    {
    public:
        struct Allocation
        {
        public:
            [[nodiscard]] bool IsValid() const noexcept { return VirtualAllocation.AllocHandle != 0 && AllocSize > 0 && BlockIndex != InvalidIndex; }
            static Allocation Invalid() { return Allocation{}; }

        public:
            Index BlockIndex = InvalidIndex;
            D3D12MA::VirtualAllocation VirtualAllocation{};

            Size Offset = 0;
            Size OffsetIndex = 0;
            Size AllocSize = 0;
            Size NumElements = 0;
        };

    private:
        struct Block
        {
            D3D12MA::VirtualBlock* VirtualBlock = nullptr;
            Size Offset = 0;
        };

    public:
        GpuStorage(RenderContext& renderContext, const GpuStorageDesc& desc);
        GpuStorage(const GpuStorage&) = delete;
        GpuStorage(GpuStorage&&) noexcept = delete;
        ~GpuStorage();

        GpuStorage& operator=(const GpuStorage&) = delete;
        GpuStorage& operator=(GpuStorage&&) noexcept = delete;

        [[nodiscard]] Allocation Allocate(const Size numElements);
        void Deallocate(const Allocation& allocation);

        void ForceReset();

        [[nodiscard]] Size GetAllocatedSize() const noexcept { return allocatedSize; }
        [[nodiscard]] Size GetBufferSize() const noexcept { return bufferSize; }
        [[nodiscard]] Size GetNumAllocatedElements() const noexcept { return allocatedSize / elementSize; }
        [[nodiscard]] GpuFence& GetStorageFence() noexcept { return fence; }
        [[nodiscard]] bool IsLinearAllocator() const noexcept { return bIsLinearAllocEnabled; }

        [[nodiscard]] Handle<GpuBuffer> GetGpuBuffer() const noexcept { return gpuBuffer; }
        [[nodiscard]] Handle<GpuView> GetSrv() const noexcept { return srv; }
        [[nodiscard]] Handle<GpuView> GetRawSrv() const noexcept { return rawSrv; }
        [[nodiscard]] Handle<GpuView> GetUav() const noexcept { return uav; }

    private:
        [[nodiscard]] GpuBufferDesc CreateBufferDesc(const U32 numElements) const noexcept;

        bool AllocateWithBlock(const Size allocSize, const Index blockIdx, Allocation& allocation);
        bool Grow(const Size newAllocSize);

    private:
        // 새롭게 할당 될 버퍼의 크기는 최소 (최소 버퍼 크기 + 할당 요청 크기)
        // NextBufferSize = max(previousBufferSize + NewAllocSize, previousBufferSize * GrowthMultiplier);
        // 내부적으로 fragmentation이 발생 할 가능성이 있음.
        // ex. 버퍼의 크기를 키운다고 해도, 할당 알고리즘 상 이전 버퍼 범위와 이후 추가된 버퍼 범위가
        // 물리적으로 연속적일지 언정, 논리적으로 불연속적인 별도의 공간으로 취급되기 때문이다.
        constexpr static Size kGrowthMultiplier = 2;

        RenderContext& renderContext;

        std::string debugName;

        U32 elementSize = 1;
        Size allocatedSize = 0;
        Size bufferSize = 0;
        bool bIsShaderReadWritable = false;
        bool bIsUavCounterEnabled = false;
        bool bIsLinearAllocEnabled = false;
        bool bIsRawBuffer = false;
        bool bCreateRawSrv = false;

        GpuFence fence;

        Handle<GpuBuffer> gpuBuffer;
        eastl::vector<Block> blocks;

        Handle<GpuView> srv;
        Handle<GpuView> rawSrv;
        Handle<GpuView> uav;
    };
} // namespace ig
