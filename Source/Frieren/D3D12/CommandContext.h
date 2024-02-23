#pragma once
#include <D3D12/Common.h>
#include <Core/String.h>
#include <Core/Container.h>

namespace fe::dx
{
	class Device;
	class DescriptorHeap;
	class GPUTexture;
	class GPUBuffer;
	class GPUView;
	class RootSignature;
	class PipelineState;
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
		void AddPendingBufferBarrier(GPUBuffer&				  targetBuffer,
									 const D3D12_BARRIER_SYNC syncBefore, const D3D12_BARRIER_SYNC syncAfter,
									 D3D12_BARRIER_ACCESS accessBefore, const D3D12_BARRIER_ACCESS accessAfter,
									 const size_t offset = 0, const size_t sizeAsBytes = std::numeric_limits<size_t>::max());
		void FlushBarriers();

		void ClearRenderTarget(const GPUView& rtv, float r = 0.f, float g = 0.f, float b = 0.f, float a = 1.f);
		void ClearDepthStencil(const GPUView& dsv, float depth = 1.f, uint8_t stencil = 0);
		void ClearDepth(const GPUView& dsv, float depth = 1.f);
		void ClearStencil(const GPUView& dsv, uint8_t stencil = 0);

		void CopyBuffer(GPUBuffer& from, GPUBuffer& to);
		void CopyBuffer(GPUBuffer& from, const size_t srcOffset, const size_t numBytes, GPUBuffer& to, const size_t dstOffset);

		void SetRootSignature(RootSignature& rootSignature);
		void SetDescriptorHeaps(const std::span<std::reference_wrapper<DescriptorHeap>> targetDescriptorHeaps);
		void SetDescriptorHeap(DescriptorHeap& descriptorHeap);
		void SetVertexBuffer(GPUBuffer& vertexBuffer);
		void SetIndexBuffer(GPUBuffer& indexBuffer);
		void SetRenderTarget(const GPUView& rtv, std::optional<std::reference_wrapper<GPUView>> dsv = std::nullopt);
		void SetPrimitiveTopology(const D3D12_PRIMITIVE_TOPOLOGY primitiveTopology);
		void SetViewport(const float topLeftX, const float topLeftY, const float width, const float height, const float minDepth = 0.f, const float maxDepth = 1.f);
		void SetScissorRect(const long left, const long top, const long right, const long bottom);

		void SetRoot32BitConstants(const uint32_t registerSlot, const uint32_t num32BitValuesToSet, const void* srcData, const uint32_t destOffsetIn32BitValues);

		template <typename T>
			requires(std::is_pod_v<T>)
		void SetRoot32BitConstants(const uint32_t registerSlot, const T& data, const uint32_t destOffsetIn32BitValues)
		{
			check(sizeof(T) % 4 == 0 && "Data loss may occur specific conditions.");
			SetRoot32BitConstants(registerSlot, sizeof(T) / 4, reinterpret_cast<const void*>(&data), destOffsetIn32BitValues);
		}

		void DrawIndexed(const uint32_t numIndices, const uint32_t indexOffset = 0, const uint32_t vertexOffset = 0);

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