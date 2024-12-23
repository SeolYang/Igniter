#pragma once
#include "Igniter/D3D12/Common.h"
#include "Igniter/D3D12/GPUTextureDesc.h"

namespace ig
{
    class GpuDevice;

    class GpuTexture final
    {
        friend class GpuDevice;

    public:
        GpuTexture(ComPtr<ID3D12Resource> textureResource);
        GpuTexture(const GpuTexture&) = delete;
        GpuTexture(GpuTexture&& other) noexcept;
        ~GpuTexture() = default;

        GpuTexture& operator=(const GpuTexture&) = delete;
        GpuTexture& operator=(GpuTexture&& other) noexcept;

        bool IsValid() const { return (allocation && resource) || resource; }
        operator bool() const { return IsValid(); }

        const GpuTextureDesc& GetDesc() const { return desc; }

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

        [[nodiscard]] size_t GetIntermediateSize() const
        {
            return GetRequiredIntermediateSize(resource.Get(), 0, static_cast<uint32_t>(desc.GetNumSubresources()));
        }

    private:
        GpuTexture(const GpuTextureDesc& newDesc, ComPtr<D3D12MA::Allocation> newAllocation, ComPtr<ID3D12Resource> newResource);

    private:
        GpuTextureDesc              desc;
        ComPtr<D3D12MA::Allocation> allocation;
        ComPtr<ID3D12Resource>      resource;
    };
} // namespace ig
