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
#include <AgilitySDK/d3dx12/d3dx12_resource_helpers.h>
#include <AgilitySDK/d3d12sdklayers.h>
#pragma warning(pop)

#include <AgilitySDK/d3d12shader.h>
#include <AgilitySDK/dxgiformat.h>
#include <dxgi.h>
#include <dxgi1_6.h>
#include <dxc/dxcapi.h>

#pragma warning(push)
#pragma warning(disable : 26827)
#pragma warning(disable : 26495)
#pragma warning(disable : 4100)
#pragma warning(disable : 4189)
#pragma warning(disable : 4324)
#pragma warning(disable : 4505)
#include <D3D12MemAlloc.h>
#pragma warning(pop)

#include <WinPixEventRuntime/pix3.h>

#include <memory>
#include <functional>
#include <variant>

namespace fe
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
	bool IsSupportGpuView(const EDescriptorHeapType descriptorHeapType, const EGpuViewType gpuViewType);

	using GpuTextureSrvDesc = std::variant<D3D12_TEX1D_SRV, D3D12_TEX2D_SRV, D3D12_TEX2DMS_SRV, D3D12_TEX3D_SRV, D3D12_TEXCUBE_SRV, D3D12_TEX1D_ARRAY_SRV, D3D12_TEX2D_ARRAY_SRV, D3D12_TEX2DMS_ARRAY_SRV, D3D12_TEXCUBE_ARRAY_SRV>;
	using GpuTextureUavDesc = std::variant<D3D12_TEX1D_UAV, D3D12_TEX2D_UAV, D3D12_TEX2DMS_UAV, D3D12_TEX3D_UAV, D3D12_TEX1D_ARRAY_UAV, D3D12_TEX2D_ARRAY_UAV, D3D12_TEX2DMS_ARRAY_UAV>;
	using GpuTextureRtvDesc = std::variant<D3D12_TEX1D_RTV, D3D12_TEX2D_RTV, D3D12_TEX2DMS_RTV, D3D12_TEX3D_RTV, D3D12_TEX1D_ARRAY_RTV, D3D12_TEX2D_ARRAY_RTV, D3D12_TEX2DMS_ARRAY_RTV>;
	using GpuTextureDsvDesc = std::variant<D3D12_TEX1D_DSV, D3D12_TEX2D_DSV, D3D12_TEX2DMS_DSV, D3D12_TEX1D_ARRAY_DSV, D3D12_TEX2D_ARRAY_DSV, D3D12_TEX2DMS_ARRAY_DSV>;

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

	struct GpuCopyableFootprints
	{
	public:
		size_t RequiredSize = 0;
		std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> Layouts;
		std::vector<uint32_t> NumRows;
		std::vector<size_t> RowSizesInBytes;
	};

	inline bool IsGreyScaleFormat(const DXGI_FORMAT format)
	{
		switch (format)
		{
			case DXGI_FORMAT_R32_FLOAT:
			case DXGI_FORMAT_R32_UINT:
			case DXGI_FORMAT_R32_SINT:
			case DXGI_FORMAT_R16_FLOAT:
			case DXGI_FORMAT_R16_UNORM:
			case DXGI_FORMAT_R16_UINT:
			case DXGI_FORMAT_R16_SNORM:
			case DXGI_FORMAT_R16_SINT:
			case DXGI_FORMAT_R8_UNORM:
			case DXGI_FORMAT_R8_UINT:
			case DXGI_FORMAT_R8_SNORM:
			case DXGI_FORMAT_R8_SINT:
			case DXGI_FORMAT_A8_UNORM:
			case DXGI_FORMAT_R1_UNORM:
			case DXGI_FORMAT_BC4_UNORM:
			case DXGI_FORMAT_BC4_SNORM:
				return true;
			default:
				return false;
		}
	}

	inline bool IsUnormFormat(const DXGI_FORMAT format)
	{
		switch (format)
		{
			case DXGI_FORMAT_R16G16B16A16_UNORM:
			case DXGI_FORMAT_R10G10B10A2_UNORM:
			case DXGI_FORMAT_R8G8B8A8_UNORM:
			case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			case DXGI_FORMAT_R16G16_UNORM:
			case DXGI_FORMAT_R8G8_UNORM:
			case DXGI_FORMAT_D16_UNORM:
			case DXGI_FORMAT_R16_UNORM:
			case DXGI_FORMAT_R8_UNORM:
			case DXGI_FORMAT_A8_UNORM:
			case DXGI_FORMAT_R1_UNORM:
			case DXGI_FORMAT_R8G8_B8G8_UNORM:
			case DXGI_FORMAT_G8R8_G8B8_UNORM:
			case DXGI_FORMAT_BC1_UNORM:
			case DXGI_FORMAT_BC1_UNORM_SRGB:
			case DXGI_FORMAT_BC2_UNORM:
			case DXGI_FORMAT_BC2_UNORM_SRGB:
			case DXGI_FORMAT_BC3_UNORM:
			case DXGI_FORMAT_BC3_UNORM_SRGB:
			case DXGI_FORMAT_BC4_UNORM:
			case DXGI_FORMAT_BC5_UNORM:
			case DXGI_FORMAT_B5G6R5_UNORM:
			case DXGI_FORMAT_B5G5R5A1_UNORM:
			case DXGI_FORMAT_B8G8R8A8_UNORM:
			case DXGI_FORMAT_B8G8R8X8_UNORM:
			case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
			case DXGI_FORMAT_BC7_UNORM:
			case DXGI_FORMAT_BC7_UNORM_SRGB:
			case DXGI_FORMAT_B4G4R4A4_UNORM:
				return true;
			default:
				return false;
		}
	}

	inline bool IsUintFormat(const DXGI_FORMAT format)
	{
		switch (format)
		{
			case DXGI_FORMAT_R32G32B32A32_UINT:
			case DXGI_FORMAT_R32G32B32_UINT:
			case DXGI_FORMAT_R16G16B16A16_UINT:
			case DXGI_FORMAT_R32G32_UINT:
			case DXGI_FORMAT_R10G10B10A2_UINT:
			case DXGI_FORMAT_R8G8B8A8_UINT:
			case DXGI_FORMAT_R16G16_UINT:
			case DXGI_FORMAT_R32_UINT:
			case DXGI_FORMAT_R8G8_UINT:
			case DXGI_FORMAT_R16_UINT:
			case DXGI_FORMAT_R8_UINT:
			case DXGI_FORMAT_FORCE_UINT:
				return true;
			default:
				return false;
		}
	}

	inline bool IsUnsignedFormat(const DXGI_FORMAT format)
	{
		return IsUnormFormat(format) || IsUintFormat(format);
	}

	inline bool IsFloatFormat(const DXGI_FORMAT format)
	{
		switch (format)
		{
			case DXGI_FORMAT_R32G32B32A32_FLOAT:
			case DXGI_FORMAT_R32G32B32_FLOAT:
			case DXGI_FORMAT_R16G16B16A16_FLOAT:
			case DXGI_FORMAT_R32G32_FLOAT:
			case DXGI_FORMAT_R11G11B10_FLOAT:
			case DXGI_FORMAT_R16G16_FLOAT:
			case DXGI_FORMAT_D32_FLOAT:
			case DXGI_FORMAT_R32_FLOAT:
			case DXGI_FORMAT_R16_FLOAT:
				return true;
			default:
				return false;
		}
	}

	void SetObjectName(ID3D12Object* object, const std::string_view name);
} // namespace fe