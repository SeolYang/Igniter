#pragma once
#include <D3D12/Common.h>
#include <D3D12/DescriptorHeap.h>
#include <D3D12/GPUTextureDesc.h>
#include <Core/Assert.h>

namespace fe
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
			check(resource);
			return *resource.Get();
		}

		auto& GetNative()
		{
			check(resource);
			return *resource.Get();
		}

	private:
		GpuTexture(const GPUTextureDesc& newDesc, ComPtr<D3D12MA::Allocation> newAllocation,
				   ComPtr<ID3D12Resource> newResource);

	private:
		GPUTextureDesc				desc;
		ComPtr<D3D12MA::Allocation> allocation;
		ComPtr<ID3D12Resource>		resource;
	};
} // namespace fe
