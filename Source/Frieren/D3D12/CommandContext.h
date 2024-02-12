#pragma once
#include <D3D12/Common.h>
#include <Core/String.h>
#include <Core/Container.h>

namespace fe::dx
{
	class Device;
	class PipelineState;
	class GPUTexture;
	class Descriptor;
	class DescriptorHeap;
	class CommandContext
	{
		friend class Device;

	public:
		using NativeType = ID3D12GraphicsCommandList7;

	public:
		CommandContext(const CommandContext&) = delete;
		CommandContext(CommandContext&& other) noexcept;
		~CommandContext() = default;

		CommandContext& operator=(const CommandContext&) = delete;
		CommandContext& operator=(CommandContext&& other) noexcept;

		auto& GetNative() { return *cmdList.Get(); }

		bool IsValid() const { return cmdAllocator && cmdList; }
		operator bool() const { return IsValid(); }

		bool IsReadyToExecute() const
		{
			return pendingGlobalBarriers.empty() && pendingTextureBarriers.empty() && pendingBufferBarriers.empty();
		}

		// Equivalent to Reset
		void Begin(PipelineState* const initState = nullptr);
		// Equivalent to Close
		void End();

		// https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html#equivalent-d3d12_barrier_sync-bit-for-each-d3d12_resource_states-bit
		void AddPendingTextureBarrier(GPUTexture& targetTexture, const D3D12_BARRIER_SYNC syncBefore,
									  const D3D12_BARRIER_SYNC syncAfter, D3D12_BARRIER_ACCESS accessBefore,
									  const D3D12_BARRIER_ACCESS accessAfter, const D3D12_BARRIER_LAYOUT layoutBefore,
									  const D3D12_BARRIER_LAYOUT			layoutAfter,
									  const D3D12_BARRIER_SUBRESOURCE_RANGE subresourceRange = { 0xffffffff, 0, 0, 0, 0,
																								 0 });
		void FlushPendingTextureBarriers();

		void ClearRenderTarget(const Descriptor& renderTargetView, float r = 0.f, float g = 0.f, float b = 0.f,
							   float a = 1.f);
		void ClearDepthStencil(const Descriptor& depthStencilView, float depth, uint8_t stencil);
		void ClearDepth(const Descriptor& depthStencilView, float depth = 1.f);
		void ClearStencil(const Descriptor& depthStencilView, uint8_t stencil = 0);

		void SetRenderTarget(const Descriptor&								   renderTargetView,
							 std::optional<std::reference_wrapper<Descriptor>> depthStencilView = std::nullopt);
		void SetDescriptorHeap(DescriptorHeap& descriptorHeap);

	private:
		CommandContext(ComPtr<ID3D12CommandAllocator> newCmdAllocator, ComPtr<NativeType> newCmdList,
					   const EQueueType targetQueueType);

	private:
		ComPtr<ID3D12CommandAllocator> cmdAllocator;
		ComPtr<NativeType>			   cmdList;
		EQueueType					   cmdListTargetQueueType;

		std::vector<D3D12_GLOBAL_BARRIER>  pendingGlobalBarriers;
		std::vector<D3D12_TEXTURE_BARRIER> pendingTextureBarriers;
		std::vector<D3D12_BUFFER_BARRIER>  pendingBufferBarriers;
	};
} // namespace fe::dx