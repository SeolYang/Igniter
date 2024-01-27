#pragma once
#include <D3D12/Common.h>
#include <D3D12/GPUResource.h>
#include <D3D12/DescriptorHeap.h>
#include <D3D12/GPUTextureDesc.h>

namespace fe
{
	class Device;
	class GPUTexture
	{
	public:
		GPUTexture(Device& device, const GPUTextureDesc& desc);
		GPUTexture(wrl::ComPtr<ID3D12Resource> existTexture);
		~GPUTexture() = default;

		GPUTexture(GPUTexture&& other) noexcept;
		GPUTexture& operator=(GPUTexture&& other) noexcept;

		const GPUTextureDesc& GetDesc() const { return desc; }
		const GPUResource&	  GetAllocation() const { return allocation; }

	private:
		GPUTextureDesc desc;
		GPUResource	   allocation;
	};
} // namespace fe
