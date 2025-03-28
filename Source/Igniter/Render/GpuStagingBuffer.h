#pragma once
#include "Igniter/Render/Common.h"

namespace ig
{
    enum class EGpuStagingBufferFlags : U8
    {
        None = 0,
    };

    IG_ENUM_FLAGS(EGpuStagingBufferFlags);

    struct GpuStagingBufferDesc
    {
        Bytes BufferSize = 0; // Assert(>0)
        EGpuStagingBufferFlags Flags = EGpuStagingBufferFlags::None;
        std::string_view DebugName = "UnnamedStagingBuffer";
    };

    class GpuBuffer;
    class RenderContext;

    class GpuStagingBuffer
    {
    public:
        GpuStagingBuffer(RenderContext& renderCtx, const GpuStagingBufferDesc& desc);
        GpuStagingBuffer(const GpuStagingBuffer&) = delete;
        GpuStagingBuffer(GpuStagingBuffer&&) noexcept = delete;
        ~GpuStagingBuffer();

        GpuStagingBuffer& operator=(const GpuStagingBuffer&) = delete;
        GpuStagingBuffer& operator=(GpuStagingBuffer&&) noexcept = delete;

        [[nodiscard]] Handle<GpuBuffer> GetBuffer(const LocalFrameIndex localFrameIdx) const noexcept { return buffer[localFrameIdx]; }
        [[nodiscard]] U8* GetMappedBuffer(const LocalFrameIndex localFrameIdx) noexcept { return mappedBuffer[localFrameIdx]; }

    private:
        RenderContext* renderCtx = nullptr;
        InFlightFramesResource<Handle<GpuBuffer>> buffer;
        InFlightFramesResource<U8*> mappedBuffer;
    };
}
