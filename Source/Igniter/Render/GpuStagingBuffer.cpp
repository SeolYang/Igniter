#include "Igniter/Igniter.h"
#include "Igniter/D3D12/GpuBuffer.h"
#include "Igniter/D3D12/GpuBufferDesc.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/GpuStagingBuffer.h"

namespace ig
{
    GpuStagingBuffer::GpuStagingBuffer(RenderContext& renderCtx, const GpuStagingBufferDesc& desc)
        : renderCtx(&renderCtx)
    {
        IG_CHECK(desc.BufferSize < 0xFFFFFFFFUi32);
        
        GpuBufferDesc bufferDesc;
        bufferDesc.AsUploadBuffer((U32)desc.BufferSize, false);
        for (const LocalFrameIndex localFrameIdx : LocalFramesView)
        {
            bufferDesc.DebugName = String(std::format("{} ({})", desc.DebugName, localFrameIdx));
            buffer[localFrameIdx] = renderCtx.CreateBuffer(bufferDesc);
            mappedBuffer[localFrameIdx] = renderCtx.Lookup(buffer[localFrameIdx])->Map();
        }
    }

    GpuStagingBuffer::~GpuStagingBuffer()
    {
        if (renderCtx == nullptr)
        {
            return;
        }

        for (const LocalFrameIndex localFrameIdx : LocalFramesView)
        {
            if (GpuBuffer* bufferPtr = renderCtx->Lookup(buffer[localFrameIdx]);
                bufferPtr != nullptr)
            {
                bufferPtr->Unmap();
            }
            renderCtx->DestroyBuffer(buffer[localFrameIdx]);
        }
    }
}
