#pragma once
#include <D3D12/DescriptorHeap.h>
#include <D3D12/GPUTextureDesc.h>

namespace fe
{
	class GPUTexture
	{
	public:
		~GPUTexture();

		GPUTexture(GPUTexture&& other) noexcept;
		GPUTexture& operator=(GPUTexture&& other) noexcept;

		const GPUTextureDesc& GetDesc() const { return desc; }
		const GPUAllocation&  GetAllocation() const { return allocation; }

	private:
		friend class Device;
		GPUTexture(const GPUTextureDesc& desc, GPUAllocation allocation);

	private:
		GPUTextureDesc desc;
		GPUAllocation  allocation;
	};
} // namespace fe
