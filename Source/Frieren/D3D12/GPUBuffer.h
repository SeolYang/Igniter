#pragma once
#include <D3D12/Common.h>
#include <D3D12/GPUResource.h>
#include <D3D12/DescriptorHeap.h>
#include <D3D12/GPUBufferDesc.h>

namespace fe
{
	class Device;
	class GPUBuffer
	{
	public:
		GPUBuffer(Device& device, const GPUBufferDesc& desc);
		~GPUBuffer();

		GPUBuffer(GPUBuffer&& other) noexcept;
		GPUBuffer& operator=(GPUBuffer&& other) noexcept;

		const GPUBufferDesc& GetDesc() const { return desc; }
		const GPUResource& GetAllocation() const { return allocation; }

	private:
		GPUBufferDesc desc;
		GPUResource allocation;
	};

} // namespace fe