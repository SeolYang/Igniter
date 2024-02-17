#pragma once
#include <D3D12/Common.h>
#include <Core/String.h>
#include <Core/Container.h>

namespace fe::dx
{
	class Device;
	class PipelineState;
	class GPUTexture;
	class GPUBuffer;
	class Descriptor;
	class DescriptorHeap;
	class RootSignature;
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
		void AddPendingTextureBarrier(GPUTexture&			   targetTexture,
									  const D3D12_BARRIER_SYNC syncBefore, const D3D12_BARRIER_SYNC syncAfter,
									  D3D12_BARRIER_ACCESS accessBefore, const D3D12_BARRIER_ACCESS accessAfter,
									  const D3D12_BARRIER_LAYOUT layoutBefore, const D3D12_BARRIER_LAYOUT layoutAfter,
									  const D3D12_BARRIER_SUBRESOURCE_RANGE subresourceRange = { 0xffffffff, 0, 0, 0, 0,
																								 0 });
		void FlushPendingTextureBarriers();
		void AddPendingBufferBarrier(GPUBuffer&				  targetBuffer,
									 const D3D12_BARRIER_SYNC syncBefore, const D3D12_BARRIER_SYNC syncAfter,
									 D3D12_BARRIER_ACCESS accessBefore, const D3D12_BARRIER_ACCESS accessAfter,
									 const size_t offset = 0, const size_t sizeAsBytes = std::numeric_limits<size_t>::max());
		void FlushPendingBufferBarriers();
		void FlushBarriers();

		void ClearRenderTarget(const Descriptor& renderTargetView, float r = 0.f, float g = 0.f, float b = 0.f,
							   float a = 1.f);
		void ClearDepthStencil(const Descriptor& depthStencilView, float depth, uint8_t stencil);
		void ClearDepth(const Descriptor& depthStencilView, float depth = 1.f);
		void ClearStencil(const Descriptor& depthStencilView, uint8_t stencil = 0);

		void SetRenderTarget(const Descriptor&								   renderTargetView,
							 std::optional<std::reference_wrapper<Descriptor>> depthStencilView = std::nullopt);
		void SetDescriptorHeaps(const std::span<std::reference_wrapper<DescriptorHeap>> targetDescriptorHeaps);
		void SetDescriptorHeap(DescriptorHeap& descriptorHeap);
		void SetRootSignature(RootSignature& rootSignature);

		void CopyBuffer(GPUBuffer& from, GPUBuffer& to);
		void CopyBuffer(GPUBuffer& from, const size_t srcOffset, const size_t numBytes, GPUBuffer& to, const size_t dstOffset);

		void SetVertexBuffer(GPUBuffer& vertexBuffer);
		void SetIndexBuffer(GPUBuffer& indexBuffer);

		void SetPrimitiveTopology(const D3D12_PRIMITIVE_TOPOLOGY primitiveTopology);
		void DrawIndexed(const uint32_t numIndices, const uint32_t indexOffset = 0, const uint32_t vertexOffset = 0);

		void SetViewport(const float topLeftX, const float topLeftY, const float width, const float height, const float minDepth = 0.f, const float maxDepth = 1.f);
		void SetScissorRect(const long left, const long top, const long right, const long bottom);

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