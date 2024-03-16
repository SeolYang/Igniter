#pragma once
#include <D3D12/Common.h>
#include <D3D12/DescriptorHeap.h>
#include <D3D12/GpuBufferDesc.h>
#include <Core/Assert.h>
#include <Core/Handle.h>

namespace ig
{
    struct MappedGpuBuffer
    {
        uint8_t* const MappedPtr = nullptr;
    };

    class RenderDevice;
    class GpuBuffer
    {
        friend class RenderDevice;
        friend class Handle<MappedGpuBuffer, GpuBuffer*>;

    public:
        GpuBuffer(ComPtr<ID3D12Resource> bufferResource);
        GpuBuffer(const GpuBuffer&) = delete;
        GpuBuffer(GpuBuffer&& other) noexcept;
        ~GpuBuffer() = default;

        GpuBuffer& operator=(const GpuBuffer&) = delete;
        GpuBuffer& operator=(GpuBuffer&& other) noexcept;

        bool IsValid() const { return (allocation && resource) || resource; }
        operator bool() const { return IsValid(); }

        const GpuBufferDesc& GetDesc() const { return desc; }

        const auto& GetNative() const
        {
            IG_CHECK(resource);
            return *resource.Get();
        }

        auto& GetNative()
        {
            IG_CHECK(resource);
            return *resource.Get();
        }

        uint8_t* Map(const uint64_t offset = 0);
        void Unmap();

        GPUResourceMapGuard MapGuard(const uint64_t offset = 0);
        Handle<MappedGpuBuffer, GpuBuffer*> MapHandle(HandleManager& handleManager, const uint64_t offset = 0);

        std::optional<D3D12_VERTEX_BUFFER_VIEW> GetVertexBufferView() const
        {
            std::optional<D3D12_VERTEX_BUFFER_VIEW> view{};
            if (desc.GetBufferType() == EGpuBufferType::VertexBuffer)
            {
                view = D3D12_VERTEX_BUFFER_VIEW{
                    .BufferLocation = resource->GetGPUVirtualAddress(),
                    .SizeInBytes = static_cast<uint32_t>(desc.GetSizeAsBytes()),
                    .StrideInBytes = desc.GetStructureByteStride()
                };
            }

            return view;
        }

        std::optional<D3D12_INDEX_BUFFER_VIEW> GetIndexBufferView() const
        {
            std::optional<D3D12_INDEX_BUFFER_VIEW> view{};
            if (desc.GetBufferType() == EGpuBufferType::IndexBuffer)
            {
                view = D3D12_INDEX_BUFFER_VIEW{
                    .BufferLocation = resource->GetGPUVirtualAddress(),
                    .SizeInBytes = static_cast<uint32_t>(desc.GetSizeAsBytes()),
                    .Format = desc.GetIndexFormat()
                };
            }

            return view;
        }

    private:
        GpuBuffer(const GpuBufferDesc& newDesc, ComPtr<D3D12MA::Allocation> newAllocation,
                  ComPtr<ID3D12Resource> newResource);

        void operator()(details::HandleImpl handle, const uint64_t evaluatedTypeHash, MappedGpuBuffer* mappedGPUBuffer);

    private:
        GpuBufferDesc desc;
        ComPtr<D3D12MA::Allocation> allocation;
        ComPtr<ID3D12Resource> resource;
    };

} // namespace ig