#pragma once
#include <D3D12/Common.h>

namespace fe::dx
{
	class GPUResource
	{
	public:
		GPUResource(D3D12MA::Allocator& allocator, const D3D12MA::ALLOCATION_DESC& allocationDesc, const D3D12_RESOURCE_DESC1& resourceDesc);
		GPUResource(ComPtr<D3D12MA::Allocation> allocation, ComPtr<ID3D12Resource> resource);
		GPUResource(ComPtr<ID3D12Resource> resource);
		GPUResource(GPUResource&& other) noexcept;
		~GPUResource() = default;

		GPUResource& operator=(GPUResource&& other) noexcept;

		void SafeRelease();

		ID3D12Resource&		 GetResource() const { return *resource.Get(); }
		D3D12MA::Allocation& GetAllocation() const { return *allocation.Get(); }

	private:
		ComPtr<D3D12MA::Allocation> allocation = {};
		ComPtr<ID3D12Resource>		resource = {};
	};
} // namespace fe::dx