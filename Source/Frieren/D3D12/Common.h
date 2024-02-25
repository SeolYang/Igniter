#pragma once
#include <AgilitySDK/d3dcommon.h>

#pragma warning(push)
#pragma warning(disable : 26827)
#include <AgilitySDK/d3d12.h>
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable : 6001)
#pragma warning(disable : 26819)
#include <AgilitySDK/d3dx12/d3dx12.h>
#include <AgilitySDK/d3d12sdklayers.h>
#pragma warning(pop)

#include <AgilitySDK/d3d12shader.h>
#include <AgilitySDK/dxgiformat.h>
#include <dxgi.h>
#include <dxgi1_6.h>
#include <directx-dxc/dxcapi.h>

#pragma warning(push)
#pragma warning(disable : 26827)
#pragma warning(disable : 26495)
#pragma warning(disable : 4100)
#pragma warning(disable : 4189)
#pragma warning(disable : 4324)
#pragma warning(disable : 4505)
#include <D3D12MemAlloc.h>
#pragma warning(pop)

#include <memory>
#include <functional>

namespace fe::dx
{
	template <typename Ty>
	using ComPtr = Microsoft::WRL::ComPtr<Ty>;

	using GPUResourceMapGuard = std::unique_ptr<uint8_t, std::function<void(uint8_t*)>>;

	enum class EQueueType
	{
		Direct,
		AsyncCompute,
		Copy
	};
	D3D12_COMMAND_LIST_TYPE ToNativeCommandListType(const EQueueType type);

	enum class EDescriptorHeapType
	{
		CBV_SRV_UAV,
		Sampler,
		RTV,
		DSV
	};
	inline D3D12_DESCRIPTOR_HEAP_TYPE ToNativeDescriptorHeapType(const EDescriptorHeapType type)
	{
		switch (type)
		{
			case EDescriptorHeapType::CBV_SRV_UAV:
				return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

			case EDescriptorHeapType::Sampler:
				return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

			case EDescriptorHeapType::RTV:
				return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

			case EDescriptorHeapType::DSV:
				return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

		}

		return D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
	}

	inline bool IsShaderVisibleDescriptorHeapType(const EDescriptorHeapType type)
	{
		return type == EDescriptorHeapType::CBV_SRV_UAV || type == EDescriptorHeapType::Sampler;
	}

	enum class EGpuViewType
	{
		ConstantBufferView,
		ShaderResourceView,
		UnorderedAccessView,
		Sampler,
		RenderTargetView,
		DepthStencilView,
		Unknown
	};
	bool IsSupportGPUView(const EDescriptorHeapType descriptorHeapType, const EGpuViewType gpuViewType);

	struct GpuViewTextureSubresource
	{
		union
		{
			/**
			 * Available for
			 * Shader Resource View: 1D, 1D Array, 2D, 2D Array, 3D, Cubemap
			 */
			uint16_t MostDetailedMip;

			/**
			 * Available for
			 * Unordered Access View: 3D
			 * Render Target View: 3D
			 */
			uint16_t FirstWSlice = 0;
		};

		union
		{
			/**
			 * Available for
			 * Shader Resource View: 1D, 1D Array, 2D, 2D Array, 3D, Cubemap
			 */
			uint16_t MipLevels;

			/**
			 * Available for
			 * Unordered Access View: 3D
			 * Render Target View: 3D
			 */
			uint16_t WSize = 0;
		};

		/**
		 * Available for
		 * Unordered Access View: 1D, 1D Array, 2D, 2D Array, 3D
		 * Render Target View: 1D, 1D Array, 2D, 2D Array, 3D
		 * Depth Stencil View: 1D, 1D Array, 2D, 2D Array
		 */
		uint16_t MipSlice = 0;

		/**
		 * Available for
		 * Shader Resource View: 2D, 2D Array
		 * Unordered Access View: 2D, 2D Array
		 * Render Target View: 2D, 2D Array
		 */
		uint16_t PlaneSlice = 0;

		/**
		 *
		 * Available for
		 * Shader Resource View: 1D Array, 2D Array, 2DMS Array
		 * Unordered Access View: 1D Array, 2D Array, 2DMS Array
		 * Render Target View: 1D Array, 2D Array, 2DMS Array
		 * Depth Stencil View: 1D Array, 2D Array, 2DMS Array
		 */
		uint16_t FirstArraySlice = 0;

		/**
		 * Available for
		 * Shader Resource View: 1D Array, 2D Array, 2DMS Array
		 * Unordered Access View: 1D Array, 2D Array, 2DMS Array
		 * Render Target View: 1D Array, 2D Array, 2DMS Array
		 * Depth Stencil View: 1D Array, 2D Array, 2DMS Array
		 */
		uint16_t ArraySize = 0;
	};

	void SetObjectName(ID3D12Object* object, const std::string_view name);
} // namespace fe::dx