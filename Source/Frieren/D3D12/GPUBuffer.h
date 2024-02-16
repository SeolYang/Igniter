#pragma once
#include <D3D12/Common.h>
#include <D3D12/DescriptorHeap.h>
#include <D3D12/GPUBufferDesc.h>

namespace fe::dx
{
	class Device;
	class GPUBuffer
	{
		friend class Device;

	public:
		GPUBuffer(ComPtr<ID3D12Resource> bufferResource);
		GPUBuffer(const GPUBuffer&) = delete;
		GPUBuffer(GPUBuffer&& other) noexcept;
		~GPUBuffer() = default;

		GPUBuffer& operator=(const GPUBuffer&) = delete;
		GPUBuffer& operator=(GPUBuffer&& other) noexcept;

		bool IsValid() const { return (allocation != nullptr && resource != nullptr) || resource != nullptr; }
		operator bool() const { return IsValid(); }

		const GPUBufferDesc& GetDesc() const { return desc; }

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

		GPUResourceMapGuard Map(const uint32_t subresource = 0, const CD3DX12_RANGE readRange = { 0, 0 });

	private:
		void Unmap();

	private:
		GPUBuffer(const GPUBufferDesc& newDesc, ComPtr<D3D12MA::Allocation> newAllocation,
				  ComPtr<ID3D12Resource> newResource);

	private:
		GPUBufferDesc				desc;
		ComPtr<D3D12MA::Allocation> allocation;
		ComPtr<ID3D12Resource>		resource;
	};

} // namespace fe::dx