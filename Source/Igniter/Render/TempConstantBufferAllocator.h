#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/Handle.h"
#include "Igniter/D3D12/GpuBufferDesc.h"
#include "Igniter/Render/Common.h"

namespace ig
{
    class GpuView;
    struct TempConstantBuffer final
    {
    public:
        TempConstantBuffer(const RenderResource<GpuView> cbv, uint8_t* const mappedPtr) : cbv(cbv), mappedPtr(mappedPtr) {}
        ~TempConstantBuffer() = default;

        template <typename T>
        void Write(const T& data)
        {
            if (mappedPtr != nullptr)
            {
                std::memcpy(mappedPtr, &data, sizeof(T));
            }
        }

        [[nodiscard]] RenderResource<GpuView> GetConstantBufferView() const { return cbv; }

    private:
        uint8_t* mappedPtr{nullptr};
        RenderResource<GpuView> cbv{};
    };

    class FrameManager;
    class GpuBuffer;
    class RenderContext;
    class CommandContext;
    class TempConstantBufferAllocator final
    {
    public:
        TempConstantBufferAllocator(RenderContext& renderContext,
            const size_t reservedBufferSizeInBytes = DefaultReservedBufferSizeInBytes);
        TempConstantBufferAllocator(const TempConstantBufferAllocator&) = delete;
        TempConstantBufferAllocator(TempConstantBufferAllocator&&) noexcept = delete;
        ~TempConstantBufferAllocator();

        TempConstantBufferAllocator& operator=(const TempConstantBufferAllocator&) = delete;
        TempConstantBufferAllocator& operator=(TempConstantBufferAllocator&&) noexcept = delete;

        TempConstantBuffer Allocate(const LocalFrameIndex localFrameIdx, const GpuBufferDesc& desc);

        template <typename T>
        TempConstantBuffer Allocate(const LocalFrameIndex localFrameIdx)
        {
            GpuBufferDesc constantBufferDesc{};
            constantBufferDesc.AsConstantBuffer<T>();
            return Allocate(localFrameIdx, constantBufferDesc);
        }

        // 이전에, 현재 시작할 프레임(local frame)에서 할당된 모든 할당을 해제한다. 프레임 시작시 반드시 호출해야함.
        void PreRender(const LocalFrameIndex localFrameIdx);

        void InitBufferStateTransition(CommandContext& cmdCtx);

        std::pair<size_t, size_t> GetUsedSizeInBytes() const;

        size_t GetReservedSizeInBytesPerFrame() const { return reservedSizeInBytesPerFrame; }

    public:
        // 실제 메모리 사용량을 프로파일링을 통해, 상황에 맞게 최적화된 값으로 설정하는 것이 좋다. (기본 값: 4 MB)
        static constexpr size_t DefaultReservedBufferSizeInBytes = 4194304;

    private:
        RenderContext& renderContext;

        size_t reservedSizeInBytesPerFrame;

        mutable eastl::array<Mutex, NumFramesInFlight> mutexes;
        eastl::array<RenderResource<GpuBuffer>, NumFramesInFlight> buffers;
        eastl::array<size_t, NumFramesInFlight> allocatedSizeInBytes{0};
        eastl::array<eastl::vector<RenderResource<GpuView>>, NumFramesInFlight> allocatedViews;
    };
}    // namespace ig