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
	class GPUBuffer;
	class GPUTexture;
	class ShaderBlob;
	class RootSignature;
	class PipelineState;
	class GPUView;
	class GPUViewManager;
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
		Renderer(const FrameManager& engineFrameManager, DeferredDeallocator& engineDefferedDeallocator, Window& window, dx::Device& device, dx::GPUViewManager& gpuViewManager);
		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		~Renderer();

		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void WaitForFences();
		void BeginFrame();
		void Render();
		void Render(World& world);
		void EndFrame();

		dx::Swapchain&	  GetSwapchain() { return *swapchain; }
		dx::CommandQueue& GetDirectCommandQueue() { return *directCmdQueue; }

	private:
		const FrameManager&	 frameManager;
		DeferredDeallocator& deferredDeallocator;
		dx::Device&			 renderDevice;
		dx::GPUViewManager&	 gpuViewManager;

		std::unique_ptr<dx::CommandQueue>		directCmdQueue;
		std::unique_ptr<dx::CommandContextPool> directCmdCtxPool;
		std::unique_ptr<dx::Swapchain>			swapchain;
		std::vector<dx::Fence>					frameFences;

#pragma region test
		// #test
		std::unique_ptr<dx::ShaderBlob>				   vs;
		std::unique_ptr<dx::ShaderBlob>				   ps;
		std::unique_ptr<dx::RootSignature>			   bindlessRootSignature;
		std::unique_ptr<dx::PipelineState>			   pso;
		std::unique_ptr<dx::GPUTexture>				   depthStencilBuffer;
		UniqueHandle<dx::GPUView, dx::GPUViewManager*> dsv;
#pragma endregion
	};
} // namespace fe