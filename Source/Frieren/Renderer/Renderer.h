#pragma once
#include <Renderer/Common.h>
#include <Core/Handle.h>

namespace fe::dx
{
	class Device;
	class CommandQueue;
	class CommandContext;
	class CommandContextPool;
	class DescriptorHeap;
	class Swapchain;
	class Fence;

#pragma region test
	// #test
	class GpuBuffer;
	class GpuTexture;
	class ShaderBlob;
	class RootSignature;
	class PipelineState;
	class GpuView;
	class GpuViewManager;
	class TempConstantBufferAllocator;
#pragma endregion
} // namespace fe::dx

namespace fe
{
	class Window;
	class World;
	class DeferredDeallocator;
	class Renderer
	{
	public:
		Renderer(const FrameManager& engineFrameManager, DeferredDeallocator& engineDefferedDeallocator, Window& window, dx::Device& device, HandleManager& handleManager, dx::GpuViewManager& gpuViewManager);
		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		~Renderer();

		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void WaitForFences();
		void BeginFrame();
		void Render(World& world);
		void EndFrame();

		dx::Swapchain&	  GetSwapchain() { return *swapchain; }
		dx::CommandQueue& GetDirectCommandQueue() { return *directCmdQueue; }

	private:
		const FrameManager&	 frameManager;
		DeferredDeallocator& deferredDeallocator;
		dx::Device&			 renderDevice;
		HandleManager&		 handleManager;
		dx::GpuViewManager&	 gpuViewManager;

		std::unique_ptr<dx::CommandQueue>		directCmdQueue;
		std::unique_ptr<dx::CommandContextPool> directCmdCtxPool;
		std::unique_ptr<dx::Swapchain>			swapchain;
		std::vector<dx::Fence>					frameFences;

		std::unique_ptr<dx::TempConstantBufferAllocator> tempConstantBufferAllocator;

#pragma region test
		// #test
		std::unique_ptr<dx::ShaderBlob>				   vs;
		std::unique_ptr<dx::ShaderBlob>				   ps;
		std::unique_ptr<dx::RootSignature>			   bindlessRootSignature;
		std::unique_ptr<dx::PipelineState>			   pso;
		std::unique_ptr<dx::GpuTexture>				   depthStencilBuffer;
		Handle<dx::GpuView, dx::GpuViewManager*> dsv;

#pragma endregion
	};
} // namespace fe