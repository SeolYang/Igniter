#pragma once
#include <D3D12/Common.h>
#include <D3D12/GPUTextureDesc.h>

namespace ig
{
    class RenderDevice;
    class GpuTexture
    {
        friend class RenderDevice;

    public:
        GpuTexture(ComPtr<ID3D12Resource> textureResource);
        GpuTexture(const GpuTexture&) = delete;
        GpuTexture(GpuTexture&& other) noexcept;
        ~GpuTexture() = default;

        GpuTexture& operator=(const GpuTexture&) = delete;
        GpuTexture& operator=(GpuTexture&& other) noexcept;

        bool IsValid() const { return (allocation && resource) || resource; }
        operator bool() const { return IsValid(); }

        const GPUTextureDesc& GetDesc() const { return desc; }

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

        [[nodiscard]] size_t GetIntermediateSize() const { return GetRequiredIntermediateSize(resource.Get(), 0, static_cast<uint32_t>(desc.GetNumSubresources())); }

    private:
        GpuTexture(const GPUTextureDesc& newDesc, ComPtr<D3D12MA::Allocation> newAllocation,
                   ComPtr<ID3D12Resource> newResource);

    private:
        GPUTextureDesc desc;
        ComPtr<D3D12MA::Allocation> allocation;
        ComPtr<ID3D12Resource> resource;
    };
} // namespace ig
