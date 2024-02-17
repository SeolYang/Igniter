#include <ImGui/ImGuiRenderer.h>
#include <ImGui/ImGuiCanvas.h>
#include <Core/Window.h>
#include <Core/FrameManager.h>
#include <D3D12/Device.h>
#include <D3D12/DescriptorHeap.h>
#include <D3D12/Swapchain.h>
#include <D3D12/CommandQueue.h>
#include <D3D12/CommandContext.h>
#include <Renderer/Renderer.h>

namespace fe
{
	ImGuiRenderer::ImGuiRenderer(const FrameManager& engineFrameManager, Window& window, dx::Device& device)
		: frameManager(engineFrameManager), descriptorHeap(std::make_unique<dx::DescriptorHeap>(device.CreateDescriptorHeap(dx::EDescriptorHeapType::CBV_SRV_UAV, 1).value()))
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		(void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		ImGui::StyleColorsDark();

		ImGui_ImplWin32_Init(window.GetNative());
		ImGui_ImplDX12_Init(&device.GetNative(), NumFramesInFlight, DXGI_FORMAT_R8G8B8A8_UNORM, &descriptorHeap->GetNative(),
							descriptorHeap->GetIndexedCPUDescriptorHandle(0), descriptorHeap->GetIndexedGPUDescriptorHandle(0));

		commandContexts.reserve(NumFramesInFlight);
		for (size_t localFrameIdx = 0; localFrameIdx < NumFramesInFlight; ++localFrameIdx)
		{
			commandContexts.emplace_back(std::make_unique<dx::CommandContext>(device.CreateCommandContext(dx::EQueueType::Direct).value()));
		}
	}

	ImGuiRenderer::~ImGuiRenderer()
	{
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();

		commandContexts.clear();
		descriptorHeap.reset();
	}

	void ImGuiRenderer::Render(ImGuiCanvas& canvas, Renderer& renderer)
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		canvas.Render();
		ImGui::Render();

		dx::CommandContext& cmdCtx = *commandContexts[frameManager.GetLocalFrameIndex()];
		cmdCtx.Begin();
		dx::Swapchain&	swapchain = renderer.GetSwapchain();
		dx::GPUTexture& backBuffer = swapchain.GetBackBuffer();

		const dx::GPUView& backBufferRTV = swapchain.GetRenderTargetView();
		// #test Test code for clear render target
		cmdCtx.SetRenderTarget(backBufferRTV);
		cmdCtx.SetDescriptorHeap(*descriptorHeap);

		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), &cmdCtx.GetNative());

		cmdCtx.AddPendingTextureBarrier(backBuffer,
										D3D12_BARRIER_SYNC_RENDER_TARGET, D3D12_BARRIER_SYNC_NONE,
										D3D12_BARRIER_ACCESS_RENDER_TARGET, D3D12_BARRIER_ACCESS_NO_ACCESS,
										D3D12_BARRIER_LAYOUT_RENDER_TARGET, D3D12_BARRIER_LAYOUT_PRESENT);

		cmdCtx.FlushBarriers();
		cmdCtx.End();

		dx::CommandQueue& directCmdQueue = renderer.GetDirectCommandQueue();
		directCmdQueue.AddPendingContext(cmdCtx);
	}
} // namespace fe