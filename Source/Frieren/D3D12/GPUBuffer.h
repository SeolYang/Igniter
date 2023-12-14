#pragma once
#include <D3D12/DescriptorHeap.h>
#include <D3D12/GPUAllocation.h>
#include <D3D12/GPUBufferDescription.h>
#include <optional>

namespace fe
{
	class GPUBuffer
	{
	public:
		~GPUBuffer();

	private:
		friend class Device;
		GPUBuffer(const GPUBufferDesc description, const Private::GPUAllocation allocation, std::optional<Descriptor> cbv, std::optional<Descriptor> srv, std::optional<Descriptor> uav);

	private:
		const GPUBufferDesc description;
		Private::GPUAllocation	   allocation;
		std::optional<Descriptor>  srv;
		std::optional<Descriptor>  cbv;
		std::optional<Descriptor>  uav;
	};

} // namespace fe