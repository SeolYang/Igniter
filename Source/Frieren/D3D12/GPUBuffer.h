#pragma once
#include <D3D12/DescriptorHeap.h>
#include <D3D12/GPUBufferDesc.h>

namespace fe
{
	class GPUBuffer
	{
	public:
		~GPUBuffer();

		GPUBuffer(GPUBuffer&& other) noexcept;
		GPUBuffer& operator=(GPUBuffer&& other) noexcept;

		const GPUBufferDesc& GetDesc() const { return desc; }
		const GPUAllocation& GetAllocation() const { return allocation; }

	private:
		friend class Device;
		GPUBuffer(const GPUBufferDesc& description, GPUAllocation allocation);

	private:
		GPUBufferDesc desc;
		GPUAllocation allocation;
	};

} // namespace fe