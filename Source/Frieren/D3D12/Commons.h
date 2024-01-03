#pragma once
#include <AgilitySDK/d3dcommon.h>
#include <AgilitySDK/d3d12.h>
#include <AgilitySDK/d3dx12/d3dx12.h>
#include <AgilitySDK/d3d12sdklayers.h>
#include <AgilitySDK/d3d12shader.h>
#include <AgilitySDK/dxgiformat.h>
#include <dxgi.h>
#include <dxgi1_6.h>
#include <directx-dxc/dxcapi.h>
#include <D3D12MemAlloc.h>

namespace fe
{
	class GPUResource
	{
	public:
		GPUResource(D3D12MA::Allocator* allocator, const D3D12_RESOURCE_DESC1& resourceDesc, const D3D12MA::ALLOCATION_DESC& allocDesc)
		{
			if (allocator != nullptr)
			{
				allocator->CreateResource3(
					&allocDesc, &resourceDesc,
					D3D12_BARRIER_LAYOUT_UNDEFINED,
					nullptr, 0, nullptr,
					&allocation,
					IID_PPV_ARGS(&resource));
			}
		}

		GPUResource(ID3D12Resource* resource, D3D12MA::Allocation* allocation)
			: resource(resource), allocation(allocation)
		{
		}

		~GPUResource()
		{
			SafeRelease();
		}

		GPUResource(GPUResource&& other) noexcept
			: resource(std::exchange(other.resource, nullptr)), allocation(std::exchange(other.allocation, nullptr))
		{
		}

		GPUResource& operator=(GPUResource&& other) noexcept
		{
			this->resource = std::exchange(other.resource, nullptr);
			this->allocation = std::exchange(other.allocation, nullptr);
			return *this;
		}

		bool IsNull() const
		{
			return resource == nullptr && allocation != nullptr;
		}

		void SafeRelease()
		{
			if (resource != nullptr)
			{
				resource->Release();
				resource = nullptr;
			}
			if (allocation != nullptr)
			{
				allocation->Release();
				allocation = nullptr;
			}
		}

		ID3D12Resource*		 GetResource() const { return resource; }
		D3D12MA::Allocation* GetAllocation() const { return allocation; }

	private:
		ID3D12Resource*		 resource = nullptr;
		D3D12MA::Allocation* allocation = nullptr;
	};
} // namespace fe

namespace fe::Private
{
	void SetD3DObjectName(ID3D12Object* object, std::string_view name);
} // namespace fe::Private

namespace fe::wrl
{
	template <typename Ty>
	using ComPtr = Microsoft::WRL::ComPtr<Ty>;
}
