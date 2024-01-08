#pragma once
#include <optional>
#include <span>

#include <Core/String.h>
#include <D3D12/Common.h>

namespace fe::Private
{
	size_t	 SizeOfTexelInBits(const DXGI_FORMAT format);
	uint16_t CalculateMaxNumMipLevels(const uint64_t width, const uint64_t height, const uint64_t depth);
	bool	 IsDepthStencilFormat(const DXGI_FORMAT format);
	bool	 IsTypelessFormat(const DXGI_FORMAT format);
} // namespace fe::Private

namespace fe
{
	class GPUTextureSubresource
	{
	public:
		union
		{
			struct
			{
				/**
				 * Available for
				 * Shader Resource View: 1D, 1D Array, 2D, 2D Array, 3D, Cubemap
				 */
				uint16_t MostDetailedMip;
			};

			/**
			 * Available for
			 * Unordered Access View: 3D
			 * Render Target View: 3D
			 */
			uint16_t FirstWSlice = 0;
		};

		union
		{
			struct
			{
				/**
				 * Available for
				 * Shader Resource View: 1D, 1D Array, 2D, 2D Array, 3D, Cubemap
				 */
				uint16_t MipLevels;
			};

			/**
			 * Available for
			 * Unordered Access View: 3D
			 * Render Target View: 3D
			 */
			uint16_t WSize = -1;
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
		uint16_t ArraySize = -1;
	};

	/**
	 * 특정한 상황이 아니라면, 텍스처의 경우 용도가 매우 다양하게 사용될 수 있기 때문에 최적(optimal)의 배리어를 가지지 못할 수도 있음.
	 * 예를 들어, 쉐이더에서 읽기/쓰기 모두 가능한(쉐이더 리소스로도 비순차 접근 리소스로도 사용 가능한) 경우, 최적의 레이아웃을
	 * 무엇으로 설정해야 하는가에 대해서 명확하지 않기 때문에 표준(최적) 상태를 제공하지 않음.
	 */
	class GPUTextureDesc
	{
	public:
		static GPUTextureDesc BuildTexture1D(const uint32_t width, const uint16_t mipLevels, const DXGI_FORMAT format, const bool bIsShaderReadWrite);
		static GPUTextureDesc BuildTexture2D(const uint32_t width, const uint32_t height, const uint16_t mipLevels, const DXGI_FORMAT format, const bool bIsShaderReadWrite, const bool bIsMSAAEnabled = false, const uint32_t sampleCount = 1, uint32_t sampleQuality = 0);
		static GPUTextureDesc BuildTexture3D(const uint32_t width, const uint32_t height, const uint32_t depth, const uint16_t mipLevels, const DXGI_FORMAT format, const bool bIsShaderReadWrite, const bool bIsMSAAEnabled = false, const uint32_t sampleCount = 1, uint32_t sampleQuality = 0);
		static GPUTextureDesc BuildRenderTarget(const uint32_t width, const uint32_t height, const DXGI_FORMAT format);
		static GPUTextureDesc BuildDepthStencil(const uint32_t width, const uint32_t height, const DXGI_FORMAT format);

		static GPUTextureDesc BuildTexture1DArray(const uint32_t width, const uint16_t arrayLength, const uint16_t mipLevels, const DXGI_FORMAT format, const bool bIsShaderReadWrite);
		static GPUTextureDesc BuildTexture2DArray(const uint32_t width, const uint32_t height, const uint16_t arrayLength, const uint16_t mipLevels, const DXGI_FORMAT format, const bool bIsShaderReadWrite, const bool bIsMSAAEnabled = false, const uint32_t sampleCount = 1, uint32_t sampleQuality = 0);
		static GPUTextureDesc BuildCubemap(const uint32_t width, const uint32_t height, const uint16_t mipLevels, const DXGI_FORMAT format, const bool bIsShaderReadWrite);

		bool IsUnorderedAccessCompatible() const;
		bool IsDepthStencilCompatible() const;
		bool IsRenderTargetCompatible() const;

		bool IsTexture1D() const { return Width > 0 && Height == 0 && Depth == 0; }
		bool IsTexture2D() const { return Width > 0 && Height > 0 && Depth == 0; }
		bool IsTexture3D() const { return Width > 0 && Height > 0 && Depth > 0; }
		bool IsTextureArray() const { return ArrayLength > 0; }

		bool IsTexture1DArray() const { return IsTexture1D() && ArrayLength > 0; }
		bool IsTexture2DArray() const { return IsTexture2D() && ArrayLength > 0; }
		bool IsTextureCube() const { return bIsCubemap && IsTexture2D() && (ArrayLength == 6); }

		D3D12_RESOURCE_DESC1							ToResourceDesc() const;
		D3D12MA::ALLOCATION_DESC						ToAllocationDesc() const;
		std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC>	ToShaderResourceViewDesc(const GPUTextureSubresource subresource) const;
		std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> ToUnorderedAccessViewDesc(const GPUTextureSubresource subresource) const;
		std::optional<D3D12_RENDER_TARGET_VIEW_DESC>	ToRenderTargetViewDesc(const GPUTextureSubresource subresource) const;
		std::optional<D3D12_DEPTH_STENCIL_VIEW_DESC>	ToDepthStencilViewDesc(const GPUTextureSubresource subresource) const;

		// #todo Typeless 포맷에 대한 typed 포맷을 지정한 view 생성

	public:
		String		DebugName = String{ "Texture" };
		uint32_t	Width = 1;
		uint32_t	Height = 0;
		uint32_t	Depth = 0;
		uint16_t	ArrayLength = 0;
		uint16_t	MipLevels = 0;
		DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;

		bool bIsCubemap = false;
		bool bIsShaderReadWrite = false;

		bool	 bIsMSAAEnabled = false;
		uint32_t SampleCount = 1;
		uint32_t SampleQuality = 0;
	};
} // namespace fe