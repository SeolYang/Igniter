#pragma once
#include <D3D12/Common.h>
#include <D3D12/DescriptorHeap.h>
#include <D3D12/GPUTextureDesc.h>
#include <Core/Assert.h>

namespace fe::dx
{
	class Device;
	class GPUTexture
	{
		friend class Device;

	public:
		GPUTexture(ComPtr<ID3D12Resource> textureResource);
		GPUTexture(const GPUTexture&) = delete;
		GPUTexture(GPUTexture&& other) noexcept;
		~GPUTexture() = default;

		GPUTexture& operator=(const GPUTexture&) = delete;
		GPUTexture& operator=(GPUTexture&& other) noexcept;

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
		GPUTexture(const GPUTextureDesc& newDesc, ComPtr<D3D12MA::Allocation> newAllocation,
				   ComPtr<ID3D12Resource> newResource);

	private:
		GPUTextureDesc				desc;
		ComPtr<D3D12MA::Allocation> allocation;
		ComPtr<ID3D12Resource>		resource;
	};
} // namespace fe::dx
