#include <D3D12/GPUResource.h>
#include <Core/Assert.h>

namespace fe::dx
{
	GPUResource::GPUResource(D3D12MA::Allocator& allocator, const D3D12MA::ALLOCATION_DESC& allocationDesc, const D3D12_RESOURCE_DESC1& resourceDesc)
	{
		FE_SUCCEEDED_ASSERT(allocator.CreateResource3(
			&allocationDesc, &resourceDesc,
			D3D12_BARRIER_LAYOUT_UNDEFINED,
			nullptr, 0, nullptr,
			allocation.ReleaseAndGetAddressOf(),
			IID_PPV_ARGS(&resource)));
	}

	GPUResource::GPUResource(ComPtr<D3D12MA::Allocation> allocation, ComPtr<ID3D12Resource> resource)
		: allocation(allocation), resource(resource)
	{
	}

	GPUResource::GPUResource(ComPtr<ID3D12Resource> resource)
		: resource(resource)
	{
	}

	GPUResource::GPUResource(GPUResource&& other) noexcept
		: resource(std::move(other.resource)), allocation(std::move(other.allocation))
	{
	}

	GPUResource& GPUResource::operator=(GPUResource&& other) noexcept
	{
		this->resource = std::move(other.resource);
		this->allocation = std::move(other.allocation);
		return *this;
	}

	void GPUResource::SafeRelease()
	{
		resource.Reset();
		allocation.Reset();
	}
} // namespace fe::dx