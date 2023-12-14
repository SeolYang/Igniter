#pragma once
#include <span>
#include <optional>

#include <Core/String.h>
#include <D3D12/Commons.h>

namespace fe::Private
{
	size_t	 SizeOfTexelInBits(const DXGI_FORMAT format);
	uint16_t CalculateMipLevels(const uint64_t width, const uint64_t height, const uint64_t depth);
	bool	 IsDepthStencilFormat(const DXGI_FORMAT format);
} // namespace fe::Private

namespace fe
{
	struct GPUTextureDesc
	{
	public:
		static GPUTextureDesc BuildTexture1D(const uint64_t width, const uint16_t mipLevels, const DXGI_FORMAT format, const bool bIsShaderReadWritable);
		// static GPUTextureDesc BuildTexture1DArray(const uint64_t width);
		static GPUTextureDesc BuildTexture2D(const uint64_t width, const uint64_t height, const uint16_t mipLevels, const DXGI_FORMAT format, const bool bIsShaderReadWritable, const bool bIsMSAAEnabled = false, const uint32_t sampleCount = 1, uint32_t sampleQuality = 0);
		// static GPUTextureDesc BuildTexture2DArray(const uint64_t width, const uint32_t height, const uint16_t mipLevels, const bool bIsShaderReadWritable, const bool bIsMSAAEnabled = false, const uint32_t sampleCount = 1, uint32_t sampleQuality = 0);
		static GPUTextureDesc BuildCubemap(const uint64_t width, const uint64_t height, const uint16_t mipLevels, const DXGI_FORMAT format, const bool bIsShaderReadWritable);
		static GPUTextureDesc BuildTexture3D(const uint64_t width, const uint64_t height, const uint64_t depth, const uint16_t mipLevels, const DXGI_FORMAT format, const bool bIsShaderReadWritable);
		static GPUTextureDesc BuildRenderTarget(const uint64_t width, const uint64_t height, const DXGI_FORMAT format);
		static GPUTextureDesc BuildDepthStencil(const uint64_t width, const uint64_t height, const DXGI_FORMAT format);

		bool IsShaderResourceCompatible() const;
		bool IsUnorderedAccessCompatible() const;
		bool IsRenderTargetCompatible() const;
		bool IsDepthStencilCompatible() const;

		bool IsTexture1D() const { return Width > 0 && Height == 0 && Depth == 0; }
		bool IsTexture2D() const { return Width > 0 && Height > 0 && Depth == 0; }
		bool IsTexture3D() const { return Width > 0 && Height > 0 && Depth > 0; }
		bool IsTextureArray() const { return !IsTexture3D() && ArrayLength > 0; }

		D3D12_RESOURCE_DESC1							AsResourceDesc() const;
		D3D12MA::ALLOCATION_DESC						AsAllocationDesc() const;
		std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC>	AsShaderResourceViewDesc() const;
		std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> AsUnorderedAccessViewDesc() const;
		std::optional<D3D12_RENDER_TARGET_VIEW_DESC> AsRenderTargetViewDesc() const;
		std::optional<D3D12_DEPTH_STENCIL_VIEW_DESC> AsDepthStencilViewDesc() const;

	public:
		String		DebugName = String{ "Texture" };
		uint64_t	Width = 1;
		uint32_t	Height = 1;
		uint16_t	Depth = 1;
		uint16_t	ArrayLength = 1;
		uint16_t	MipLevels = 0;
		DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;

		bool bIsCubemap = false;
		bool bIsShaderReadWritable = false;

		bool	 bIsMSAAEnabled = false;
		uint32_t SampleCount = 1;
		uint32_t SampleQuality = 0;

		D3D12_TEXTURE_BARRIER StandardBarrier = {};
	};
} // namespace fe