#pragma once
#include <D3D12/Common.h>

namespace fe::dx
{
	class GPUResource
	{
	public:
		GPUResource(D3D12MA::Allocator& allocator, const D3D12MA::ALLOCATION_DESC& allocationDesc, const D3D12_RESOURCE_DESC1& resourceDesc)
		{
			ThrowIfFailed(allocator.CreateResource3(
				&allocationDesc, &resourceDesc,
				D3D12_BARRIER_LAYOUT_UNDEFINED,
				nullptr, 0, nullptr,
				allocation.ReleaseAndGetAddressOf(),
				IID_PPV_ARGS(&resource)));
		}

		GPUResource(ComPtr<D3D12MA::Allocation> allocation, ComPtr<ID3D12Resource> resource)
			: allocation(allocation), resource(resource)
		{
		}

		GPUResource(ComPtr<ID3D12Resource> resource)
			: resource(resource)
		{
		}

		~GPUResource() = default;

		GPUResource(GPUResource&& other) noexcept
			: resource(std::move(other.resource)), allocation(std::move(other.allocation))
		{
		}

		GPUResource& operator=(GPUResource&& other) noexcept
		{
			this->resource = std::move(other.resource);
			this->allocation = std::move(other.allocation);
			return *this;
		}

		void SafeRelease()
		{
			resource.Reset();
			allocation.Reset();
		}

		ID3D12Resource&		 GetResource() const { return *resource.Get(); }
		D3D12MA::Allocation& GetAllocation() const { return *allocation.Get(); }

	private:
		ComPtr<D3D12MA::Allocation> allocation = {};
		ComPtr<ID3D12Resource>		 resource = {};
	};
} // namespace fe