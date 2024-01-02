#include <Core/Assert.h>
#include <D3D12/GPUTextureDesc.h>

namespace fe::Private
{
	size_t SizeOfTexelInBits(const DXGI_FORMAT format)
	{
		switch (static_cast<int>(format))
		{
			case DXGI_FORMAT_R32G32B32A32_TYPELESS:
			case DXGI_FORMAT_R32G32B32A32_FLOAT:
			case DXGI_FORMAT_R32G32B32A32_UINT:
			case DXGI_FORMAT_R32G32B32A32_SINT:
				return 128;

			case DXGI_FORMAT_R32G32B32_TYPELESS:
			case DXGI_FORMAT_R32G32B32_FLOAT:
			case DXGI_FORMAT_R32G32B32_UINT:
			case DXGI_FORMAT_R32G32B32_SINT:
				return 96;

			case DXGI_FORMAT_R16G16B16A16_TYPELESS:
			case DXGI_FORMAT_R16G16B16A16_FLOAT:
			case DXGI_FORMAT_R16G16B16A16_UNORM:
			case DXGI_FORMAT_R16G16B16A16_UINT:
			case DXGI_FORMAT_R16G16B16A16_SNORM:
			case DXGI_FORMAT_R16G16B16A16_SINT:
			case DXGI_FORMAT_R32G32_TYPELESS:
			case DXGI_FORMAT_R32G32_FLOAT:
			case DXGI_FORMAT_R32G32_UINT:
			case DXGI_FORMAT_R32G32_SINT:
			case DXGI_FORMAT_R32G8X24_TYPELESS:
			case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
			case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
			case DXGI_FORMAT_Y416:
			case DXGI_FORMAT_Y210:
			case DXGI_FORMAT_Y216:
				return 64;

			case DXGI_FORMAT_R10G10B10A2_TYPELESS:
			case DXGI_FORMAT_R10G10B10A2_UNORM:
			case DXGI_FORMAT_R10G10B10A2_UINT:
			case DXGI_FORMAT_R11G11B10_FLOAT:
			case DXGI_FORMAT_R8G8B8A8_TYPELESS:
			case DXGI_FORMAT_R8G8B8A8_UNORM:
			case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			case DXGI_FORMAT_R8G8B8A8_UINT:
			case DXGI_FORMAT_R8G8B8A8_SNORM:
			case DXGI_FORMAT_R8G8B8A8_SINT:
			case DXGI_FORMAT_R16G16_TYPELESS:
			case DXGI_FORMAT_R16G16_FLOAT:
			case DXGI_FORMAT_R16G16_UNORM:
			case DXGI_FORMAT_R16G16_UINT:
			case DXGI_FORMAT_R16G16_SNORM:
			case DXGI_FORMAT_R16G16_SINT:
			case DXGI_FORMAT_R32_TYPELESS:
			case DXGI_FORMAT_D32_FLOAT:
			case DXGI_FORMAT_R32_FLOAT:
			case DXGI_FORMAT_R32_UINT:
			case DXGI_FORMAT_R32_SINT:
			case DXGI_FORMAT_R24G8_TYPELESS:
			case DXGI_FORMAT_D24_UNORM_S8_UINT:
			case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
			case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
			case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
			case DXGI_FORMAT_R8G8_B8G8_UNORM:
			case DXGI_FORMAT_G8R8_G8B8_UNORM:
			case DXGI_FORMAT_B8G8R8A8_UNORM:
			case DXGI_FORMAT_B8G8R8X8_UNORM:
			case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
			case DXGI_FORMAT_B8G8R8A8_TYPELESS:
			case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			case DXGI_FORMAT_B8G8R8X8_TYPELESS:
			case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
			case DXGI_FORMAT_AYUV:
			case DXGI_FORMAT_Y410:
			case DXGI_FORMAT_YUY2:
				return 32;

			case DXGI_FORMAT_P010:
			case DXGI_FORMAT_P016:
				return 24;

			case DXGI_FORMAT_R8G8_TYPELESS:
			case DXGI_FORMAT_R8G8_UNORM:
			case DXGI_FORMAT_R8G8_UINT:
			case DXGI_FORMAT_R8G8_SNORM:
			case DXGI_FORMAT_R8G8_SINT:
			case DXGI_FORMAT_R16_TYPELESS:
			case DXGI_FORMAT_R16_FLOAT:
			case DXGI_FORMAT_D16_UNORM:
			case DXGI_FORMAT_R16_UNORM:
			case DXGI_FORMAT_R16_UINT:
			case DXGI_FORMAT_R16_SNORM:
			case DXGI_FORMAT_R16_SINT:
			case DXGI_FORMAT_B5G6R5_UNORM:
			case DXGI_FORMAT_B5G5R5A1_UNORM:
			case DXGI_FORMAT_A8P8:
			case DXGI_FORMAT_B4G4R4A4_UNORM:
				return 16;

			case DXGI_FORMAT_NV12:
			case DXGI_FORMAT_420_OPAQUE:
			case DXGI_FORMAT_NV11:
				return 12;

			case DXGI_FORMAT_R8_TYPELESS:
			case DXGI_FORMAT_R8_UNORM:
			case DXGI_FORMAT_R8_UINT:
			case DXGI_FORMAT_R8_SNORM:
			case DXGI_FORMAT_R8_SINT:
			case DXGI_FORMAT_A8_UNORM:
			case DXGI_FORMAT_AI44:
			case DXGI_FORMAT_IA44:
			case DXGI_FORMAT_P8:
				return 8;

			case DXGI_FORMAT_R1_UNORM:
				return 1;

			case DXGI_FORMAT_BC1_TYPELESS:
			case DXGI_FORMAT_BC1_UNORM:
			case DXGI_FORMAT_BC1_UNORM_SRGB:
			case DXGI_FORMAT_BC4_TYPELESS:
			case DXGI_FORMAT_BC4_UNORM:
			case DXGI_FORMAT_BC4_SNORM:
				return 4;

			case DXGI_FORMAT_BC2_TYPELESS:
			case DXGI_FORMAT_BC2_UNORM:
			case DXGI_FORMAT_BC2_UNORM_SRGB:
			case DXGI_FORMAT_BC3_TYPELESS:
			case DXGI_FORMAT_BC3_UNORM:
			case DXGI_FORMAT_BC3_UNORM_SRGB:
			case DXGI_FORMAT_BC5_TYPELESS:
			case DXGI_FORMAT_BC5_UNORM:
			case DXGI_FORMAT_BC5_SNORM:
			case DXGI_FORMAT_BC6H_TYPELESS:
			case DXGI_FORMAT_BC6H_UF16:
			case DXGI_FORMAT_BC6H_SF16:
			case DXGI_FORMAT_BC7_TYPELESS:
			case DXGI_FORMAT_BC7_UNORM:
			case DXGI_FORMAT_BC7_UNORM_SRGB:
				return 8;

			default:
				return 0;
		}
	}

	uint16_t CalculateMaxNumMipLevels(const uint64_t width, const uint64_t height, const uint64_t depth)
	{
		uint64_t highest = width > height ? width : height;
		highest = highest > depth ? highest : depth;
		return static_cast<uint16_t>(std::log2(highest)) + 1;
	}

	bool IsDepthStencilFormat(const DXGI_FORMAT format)
	{
		return format == DXGI_FORMAT_D16_UNORM ||
			   format == DXGI_FORMAT_D24_UNORM_S8_UINT ||
			   format == DXGI_FORMAT_D32_FLOAT ||
			   format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	}

	bool IsTypelessFormat(const DXGI_FORMAT format)
	{
		return format == DXGI_FORMAT_R32G32B32A32_TYPELESS || format == DXGI_FORMAT_R32G32B32_TYPELESS ||
			   format == DXGI_FORMAT_R16G16B16A16_TYPELESS || format == DXGI_FORMAT_R32G32_TYPELESS ||
			   format == DXGI_FORMAT_R32G8X24_TYPELESS || format == DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS ||
			   format == DXGI_FORMAT_X32_TYPELESS_G8X24_UINT || format == DXGI_FORMAT_R10G10B10A2_TYPELESS ||
			   format == DXGI_FORMAT_R8G8B8A8_TYPELESS || format == DXGI_FORMAT_R16G16_TYPELESS ||
			   format == DXGI_FORMAT_R32_TYPELESS ||
			   format == DXGI_FORMAT_R24G8_TYPELESS || format == DXGI_FORMAT_R24_UNORM_X8_TYPELESS ||
			   format == DXGI_FORMAT_X24_TYPELESS_G8_UINT || format == DXGI_FORMAT_R8G8_TYPELESS ||
			   format == DXGI_FORMAT_R16_TYPELESS || format == DXGI_FORMAT_R8_TYPELESS ||
			   format == DXGI_FORMAT_BC1_TYPELESS || format == DXGI_FORMAT_BC2_TYPELESS ||
			   format == DXGI_FORMAT_BC3_TYPELESS || format == DXGI_FORMAT_BC4_TYPELESS ||
			   format == DXGI_FORMAT_BC5_TYPELESS || format == DXGI_FORMAT_R8G8B8A8_TYPELESS ||
			   format == DXGI_FORMAT_B8G8R8X8_TYPELESS || format == DXGI_FORMAT_BC6H_TYPELESS ||
			   format == DXGI_FORMAT_BC7_TYPELESS;
	}
} // namespace fe::Private

namespace fe
{
	GPUTextureDesc GPUTextureDesc::BuildTexture1D(const uint32_t width, const uint16_t mipLevels, const DXGI_FORMAT format, const bool bIsShaderReadWrite)
	{
		return {
			.DebugName = String{ "Texture1D" },
			.Width = width,
			.MipLevels = mipLevels,
			.Format = format,
			.bIsShaderReadWrite = bIsShaderReadWrite
		};
	}

	GPUTextureDesc GPUTextureDesc::BuildTexture2D(const uint32_t width, const uint32_t height, const uint16_t mipLevels, const DXGI_FORMAT format, const bool bIsShaderReadWrite, const bool bIsMSAAEnabled /*= false*/, const uint32_t sampleCount /*= 1*/, uint32_t sampleQuality /*= 0*/)
	{
		return {
			.DebugName = String{ "Texture2D" },
			.Width = width,
			.Height = height,
			.MipLevels = mipLevels,
			.Format = format,
			.bIsShaderReadWrite = bIsShaderReadWrite,
			.bIsMSAAEnabled = bIsMSAAEnabled,
			.SampleCount = bIsMSAAEnabled ? sampleCount : 1,
			.SampleQuality = bIsMSAAEnabled ? sampleQuality : 0
		};
	}

	GPUTextureDesc GPUTextureDesc::BuildTexture3D(const uint32_t width, const uint32_t height, const uint32_t depth, const uint16_t mipLevels, const DXGI_FORMAT format, const bool bIsShaderReadWrite, const bool bIsMSAAEnabled, const uint32_t sampleCount, uint32_t sampleQuality)
	{
		return {
			.DebugName = String{ "Texture2D" },
			.Width = width,
			.Height = height,
			.Depth = depth,
			.MipLevels = mipLevels,
			.Format = format,
			.bIsShaderReadWrite = bIsShaderReadWrite,
			.bIsMSAAEnabled = bIsMSAAEnabled,
			.SampleCount = bIsMSAAEnabled ? sampleCount : 1,
			.SampleQuality = bIsMSAAEnabled ? sampleQuality : 0
		};
	}

	GPUTextureDesc GPUTextureDesc::BuildRenderTarget(const uint32_t width, const uint32_t height, const DXGI_FORMAT format)
	{
		return {
			.DebugName = String{ "RenderTarget" },
			.Width = width,
			.Height = height,
			.Format = format
		};
	}

	GPUTextureDesc GPUTextureDesc::BuildDepthStencil(const uint32_t width, const uint32_t height, const DXGI_FORMAT format)
	{
		// #todo Assertion on non depth-stencil formats.
		return {
			.DebugName = String{ "DepthStencil" },
			.Width = width,
			.Height = height,
			.Format = format
		};
	}

	GPUTextureDesc GPUTextureDesc::BuildTexture1DArray(const uint32_t width, const uint16_t arrayLength, const uint16_t mipLevels, const DXGI_FORMAT format, const bool bIsShaderReadWrite)
	{
		return {
			.DebugName = String{ "Texture1DArray" },
			.Width = width,
			.ArrayLength = arrayLength,
			.MipLevels = mipLevels,
			.Format = format,
			.bIsShaderReadWrite = bIsShaderReadWrite
		};
	}

	GPUTextureDesc GPUTextureDesc::BuildTexture2DArray(const uint32_t width, const uint32_t height, const uint16_t arrayLength, const uint16_t mipLevels, const DXGI_FORMAT format, const bool bIsShaderReadWrite, const bool bIsMSAAEnabled /*= false*/, const uint32_t sampleCount /*= 1*/, uint32_t sampleQuality /*= 0*/)
	{
		return {
			.DebugName = String{ "Texture2D" },
			.Width = width,
			.Height = height,
			.ArrayLength = arrayLength,
			.MipLevels = mipLevels,
			.Format = format,
			.bIsShaderReadWrite = bIsShaderReadWrite,
			.bIsMSAAEnabled = bIsMSAAEnabled,
			.SampleCount = bIsMSAAEnabled ? sampleCount : 1,
			.SampleQuality = bIsMSAAEnabled ? sampleQuality : 0
		};
	}

	GPUTextureDesc GPUTextureDesc::BuildCubemap(const uint32_t width, const uint32_t height, const uint16_t mipLevels, const DXGI_FORMAT format, const bool bIsShaderReadWrite)
	{
		return {
			.DebugName = String{ "Cubemap" },
			.Width = width,
			.Height = height,
			.ArrayLength = 6,
			.MipLevels = mipLevels,
			.Format = format,
			.bIsCubemap = true,
			.bIsShaderReadWrite = bIsShaderReadWrite
		};
	}

	bool GPUTextureDesc::IsUnorderedAccessCompatible() const
	{
		return bIsShaderReadWrite;
	}

	bool GPUTextureDesc::IsDepthStencilCompatible() const
	{
		return !IsTexture3D() && Private::IsDepthStencilFormat(Format);
	}

	bool GPUTextureDesc::IsRenderTargetCompatible() const
	{
		return !IsUnorderedAccessCompatible() && !IsDepthStencilCompatible();
	}

	D3D12_RESOURCE_DESC1 GPUTextureDesc::ToResourceDesc() const
	{
		FE_ASSERT(Width > 0, "Invalid width.");
		D3D12_RESOURCE_DESC1 desc{};
		desc.Dimension = IsTexture1D() ? D3D12_RESOURCE_DIMENSION_TEXTURE1D : (IsTexture2D() ? D3D12_RESOURCE_DIMENSION_TEXTURE2D : (IsTexture3D() ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_UNKNOWN));
		/** resource alignment: https://asawicki.info/news_1726_secrets_of_direct3d_12_resource_alignment */
		desc.Alignment = 0;
		desc.Width = Width;
		desc.Height = Height;
		desc.DepthOrArraySize = Depth;
		desc.MipLevels = MipLevels;
		desc.Format = Format;
		desc.SampleDesc.Count = bIsMSAAEnabled ? SampleCount : 1;
		desc.SampleDesc.Quality = bIsMSAAEnabled ? SampleQuality : 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

		return desc;
	}

	D3D12MA::ALLOCATION_DESC GPUTextureDesc::ToAllocationDesc() const
	{
		D3D12MA::ALLOCATION_DESC desc{};
		desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
		return desc;
	}

	std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> GPUTextureDesc::ToShaderResourceViewDesc(const GPUTextureSubresource subresource) const
	{
		if (!Private::IsTypelessFormat(Format))
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
			desc.Format = Format;
			desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

			if (IsTexture1D())
			{
				if (IsTextureArray())
				{
					desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
					desc.Texture1DArray.FirstArraySlice = subresource.FirstArraySlice;
					desc.Texture1DArray.ArraySize = subresource.ArraySize;
					desc.Texture1DArray.MostDetailedMip = subresource.MostDetailedMip;
					desc.Texture1DArray.MipLevels = subresource.MipLevels;
				}
				else
				{
					desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
					desc.Texture1D.MostDetailedMip = subresource.MostDetailedMip;
					desc.Texture1D.MipLevels = subresource.MipLevels;
				}
			}
			else if (IsTexture2D())
			{
				if (IsTextureCube())
				{
					// #todo Handle texture cube array?
					desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
					desc.TextureCube.MostDetailedMip = subresource.MostDetailedMip;
					desc.TextureCube.MipLevels = subresource.MipLevels;
				}
				else if (IsTextureArray())
				{
					if (bIsMSAAEnabled)
					{
						desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
						desc.Texture2DMSArray.ArraySize = subresource.ArraySize;
						desc.Texture2DMSArray.FirstArraySlice = subresource.FirstArraySlice;
					}
					else
					{
						desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
						desc.Texture2DArray.ArraySize = subresource.ArraySize;
						desc.Texture2DArray.FirstArraySlice = subresource.FirstArraySlice;
						desc.Texture2DArray.MipLevels = subresource.MipLevels;
						desc.Texture2DArray.MostDetailedMip = subresource.MostDetailedMip;
						desc.Texture2DArray.PlaneSlice = subresource.PlaneSlice;
					}
				}
				else
				{
					if (bIsMSAAEnabled)
					{
						desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
					}
					else
					{
						desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
						desc.Texture2D.MipLevels = subresource.MipLevels;
						desc.Texture2D.MostDetailedMip = subresource.MostDetailedMip;
						desc.Texture2D.PlaneSlice = subresource.PlaneSlice;
					}
				}
			}
			else if (IsTexture3D())
			{
				desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
				desc.Texture3D.MipLevels = subresource.MipLevels;
				desc.Texture3D.MostDetailedMip = subresource.MostDetailedMip;
			}
			else
			{
				return std::nullopt;
			}

			return desc;
		}

		return std::nullopt;
	}

	std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> GPUTextureDesc::ToUnorderedAccessViewDesc(const GPUTextureSubresource subresource) const
	{
		if (IsUnorderedAccessCompatible() && !Private::IsTypelessFormat(Format))
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
			desc.Format = Format;

			if (IsTexture1D())
			{
				if (IsTextureArray())
				{
					desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
					desc.Texture1DArray.MipSlice = subresource.MipSlice;
					desc.Texture1DArray.FirstArraySlice = subresource.FirstArraySlice;
					desc.Texture1DArray.ArraySize = subresource.ArraySize;
				}
				else
				{
					desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
					desc.Texture1D.MipSlice = subresource.MipSlice;
				}
			}
			else if (IsTexture2D())
			{
				if (IsTextureArray())
				{
					if (bIsMSAAEnabled)
					{
						desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY;
						desc.Texture2DMSArray.ArraySize = subresource.ArraySize;
						desc.Texture2DMSArray.FirstArraySlice = subresource.FirstArraySlice;
					}
					else
					{
						desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
						desc.Texture2DArray.ArraySize = subresource.ArraySize;
						desc.Texture2DArray.FirstArraySlice = subresource.FirstArraySlice;
						desc.Texture2DArray.MipSlice = subresource.MipSlice;
						desc.Texture2DArray.PlaneSlice = subresource.PlaneSlice;
					}
				}
				else
				{
					if (bIsMSAAEnabled)
					{
						desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMS;
					}
					else
					{
						desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
						desc.Texture2D.MipSlice = subresource.MipSlice;
						desc.Texture2D.PlaneSlice = subresource.PlaneSlice;
					}
				}
			}
			else if (IsTexture3D())
			{
				desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
				desc.Texture3D.FirstWSlice = subresource.FirstWSlice;
				desc.Texture3D.WSize = subresource.WSize;
				desc.Texture3D.MipSlice = subresource.MipSlice;
			}
		}

		return std::nullopt;
	}

	std::optional<D3D12_RENDER_TARGET_VIEW_DESC> GPUTextureDesc::ToRenderTargetViewDesc(const GPUTextureSubresource subresource) const
	{
		if (IsRenderTargetCompatible() && !Private::IsTypelessFormat(Format))
		{
			D3D12_RENDER_TARGET_VIEW_DESC desc{};
			desc.Format = Format;

			if (IsTexture1D())
			{
				if (IsTextureArray())
				{
					desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
					desc.Texture1DArray.ArraySize = subresource.ArraySize;
					desc.Texture1DArray.FirstArraySlice = subresource.FirstArraySlice;
					desc.Texture1DArray.MipSlice = subresource.MipSlice;
				}
				else
				{
					desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
					desc.Texture1D.MipSlice = subresource.MipSlice;
				}
			}
			else if (IsTexture2D())
			{
				if (IsTextureArray())
				{
					if (bIsMSAAEnabled)
					{
						desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
						desc.Texture2DMSArray.ArraySize = subresource.ArraySize;
						desc.Texture2DMSArray.FirstArraySlice = subresource.FirstArraySlice;
					}
					else
					{
						desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
						desc.Texture2DArray.ArraySize = subresource.ArraySize;
						desc.Texture2DArray.FirstArraySlice = subresource.FirstArraySlice;
						desc.Texture2DArray.MipSlice = subresource.MipSlice;
						desc.Texture2DArray.PlaneSlice = subresource.PlaneSlice;
					}
				}
				else
				{
					if (bIsMSAAEnabled)
					{
						desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
					}
					else
					{
						desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
						desc.Texture2D.MipSlice = subresource.MipSlice;
						desc.Texture2D.PlaneSlice = subresource.PlaneSlice;
					}
				}
			}
			else if (IsTexture3D())
			{
				desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
				desc.Texture3D.MipSlice = subresource.MipSlice;
				desc.Texture3D.FirstWSlice = subresource.FirstWSlice;
				desc.Texture3D.WSize = subresource.WSize;
			}
		}

		return std::nullopt;
	}

	std::optional<D3D12_DEPTH_STENCIL_VIEW_DESC> GPUTextureDesc::ToDepthStencilViewDesc(const GPUTextureSubresource subresource) const
	{
		if (IsDepthStencilCompatible())
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC desc{};
			desc.Format = Format;

			if (IsTexture1D())
			{
				if (IsTextureArray())
				{
					desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
					desc.Texture1DArray.ArraySize = subresource.ArraySize;
					desc.Texture1DArray.FirstArraySlice = subresource.FirstArraySlice;
					desc.Texture1DArray.MipSlice = subresource.MipSlice;
				}
				else
				{
					desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
					desc.Texture1D.MipSlice = subresource.MipSlice;
				}
			}
			else if (IsTexture2D())
			{
				if (IsTextureArray())
				{
					if (bIsMSAAEnabled)
					{
						desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
						desc.Texture2DMSArray.ArraySize = subresource.ArraySize;
						desc.Texture2DMSArray.FirstArraySlice = subresource.FirstArraySlice;
					}
					else
					{
						desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
						desc.Texture2DArray.ArraySize = subresource.ArraySize;
						desc.Texture2DArray.FirstArraySlice = subresource.FirstArraySlice;
						desc.Texture2DArray.MipSlice = subresource.MipSlice;
					}
				}
				else
				{
					if (bIsMSAAEnabled)
					{
						desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
					}
					else
					{
						desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
						desc.Texture2D.MipSlice = subresource.MipSlice;
					}
				}
			}
		}

		return std::nullopt;
	}
} // namespace fe