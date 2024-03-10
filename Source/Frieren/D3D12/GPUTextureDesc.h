#pragma once
#include <optional>
#include <span>

#include <Core/String.h>
#include <D3D12/Common.h>

namespace fe
{
	size_t	 SizeOfTexelInBits(const DXGI_FORMAT format);
	uint16_t CalculateMaxNumMipLevels(const uint64_t width, const uint64_t height, const uint64_t depth);
	bool	 IsDepthStencilFormat(const DXGI_FORMAT format);
	bool	 IsTypelessFormat(const DXGI_FORMAT format);

	// #todo Simultaneous Access flag for resource aka. D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS
	class GPUTextureDesc : public D3D12_RESOURCE_DESC1
	{
	public:
		GPUTextureDesc() = default;
		virtual ~GPUTextureDesc() = default;

		void AsTexture1D(const uint32_t width, const uint16_t mipLevels, const DXGI_FORMAT format,
						 const bool bEnableShaderReadWrite = false, const bool bEnableSimultaneousAccess = false);
		void AsTexture2D(const uint32_t width, const uint32_t height, const uint16_t mipLevels,
						 const DXGI_FORMAT format, const bool bEnableShaderReadWrite = false,
						 const bool bEnableSimultaneousAccess = false, const bool bEnableMSAA = false,
						 const uint32_t sampleCount = 1, uint32_t sampleQuality = 0);
		void AsTexture3D(const uint32_t width, const uint32_t height, const uint16_t depth, const uint16_t mipLevels,
						 const DXGI_FORMAT format, const bool bEnableShaderReadWrite = false,
						 const bool bEnableSimultaneousAccess = false, const bool bEnableMSAA = false,
						 const uint32_t sampleCount = 1, uint32_t sampleQuality = 0);

		void AsRenderTarget(const uint32_t width, const uint32_t height, const uint16_t mipLevels,
							const DXGI_FORMAT format, const bool bEnableSimultaneousAccess = false,
							const bool bEnableMSAA = false, const uint32_t sampleCount = 1, uint32_t sampleQuality = 0);
		void AsDepthStencil(const uint32_t width, const uint32_t height, const DXGI_FORMAT format);

		void AsTexture1DArray(const uint32_t width, const uint16_t arrayLength, const uint16_t mipLevels,
							  const DXGI_FORMAT format, const bool bEnableShaderReadWrite = false,
							  const bool bEnableSimultaneousAccess = false);
		void AsTexture2DArray(const uint32_t width, const uint32_t height, const uint16_t arrayLength,
							  const uint16_t mipLevels, const DXGI_FORMAT format,
							  const bool bEnableShaderReadWrite = false, const bool bEnableSimultaneousAccess = false,
							  const bool bEnableMSAA = false, const uint32_t sampleCount = 1,
							  uint32_t sampleQuality = 0);

		void AsCubemap(const uint32_t width, const uint32_t height, const uint16_t mipLevels, const DXGI_FORMAT format,
					   const bool bEnableShaderReadWrite = false, const bool bEnableSimultaneousAccess = false,
					   const bool bEnableMSAA = false, const uint32_t sampleCount = 1, uint32_t sampleQuality = 0);

		bool IsUnorderedAccessCompatible() const;
		bool IsDepthStencilCompatible() const;
		bool IsRenderTargetCompatible() const;

		bool IsTexture1D() const { return Width > 0 && Height == 1 && DepthOrArraySize == 1; }
		bool IsTexture2D() const { return Width > 0 && Height > 0 && DepthOrArraySize == 1; }
		bool IsTexture3D() const { return Width > 0 && Height > 0 && DepthOrArraySize > 0; }
		bool IsTextureArray() const { return bIsArray; }
		bool IsTexture1DArray() const { return IsTextureArray() && Width > 0 && Height == 1 && DepthOrArraySize > 0; }
		bool IsTexture2DArray() const { return IsTextureArray() && Width > 0 && Height > 0 && DepthOrArraySize > 0; }
		bool IsTextureCube() const { return bIsCubemap; }

		bool IsMSAAEnabled() const { return bIsMSAAEnabled; }
		bool IsAllowSimultaneousAccess() const { return bIsAllowSimultaneousAccess; }

		/* #wip GpuViewTexture Subresource 손보기 */
		D3D12MA::ALLOCATION_DESC						GetAllocationDesc() const;
		std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC>	ToShaderResourceViewDesc(const GpuViewTextureSubresource& subresource) const;
		std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> ToUnorderedAccessViewDesc(const GpuViewTextureSubresource& subresource) const;
		std::optional<D3D12_RENDER_TARGET_VIEW_DESC>	ToRenderTargetViewDesc(const GpuViewTextureSubresource& subresource) const;
		std::optional<D3D12_DEPTH_STENCIL_VIEW_DESC>	ToDepthStencilViewDesc(const GpuViewTextureSubresource& subresource) const;

		std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> ConvertToNativeDesc(const GpuTextureSrvDesc& srvDesc) const;
		std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> ConvertToNativeDesc(const GpuTextureUavDesc& uavDesc) const;
		std::optional<D3D12_RENDER_TARGET_VIEW_DESC> ConvertToNativeDesc(const GpuTextureRtvDesc& rtvDesc) const;
		std::optional<D3D12_DEPTH_STENCIL_VIEW_DESC> ConvertToNativeDesc(const GpuTextureDsvDesc& DSVdESC) CONST;

		// #todo Typeless 포맷에 대한 typed 포맷을 지정한 view 생성
		void From(const D3D12_RESOURCE_DESC& desc);

		size_t GetNumSubresources() const { return (bIsArray ? static_cast<size_t>(DepthOrArraySize) : 1) * MipLevels; }

	public:
		String DebugName = String{ "Unknown Texture" };
		D3D12_BARRIER_LAYOUT InitialLayout = D3D12_BARRIER_LAYOUT_COMMON;

	private:
		bool bIsArray = false;
		bool bIsMSAAEnabled = false;
		bool bIsCubemap = false;
		bool bIsShaderReadWrite = false;
		bool bIsAllowSimultaneousAccess = false;
	};
} // namespace fe