#include <ImGui/ImGuiRenderer.h>
#include <ImGui/ImGuiCanvas.h>
#include <Core/Window.h>
#include <Core/FrameManager.h>
#include <D3D12/RenderDevice.h>
#include <D3D12/DescriptorHeap.h>
#include <D3D12/Swapchain.h>
#include <D3D12/CommandQueue.h>
#include <D3D12/CommandContext.h>
#include <Render/Renderer.h>

namespace fe
{
	ImGuiRenderer::ImGuiRenderer(const FrameManager& engineFrameManager, Window& window, RenderDevice& device)
		: frameManager(engineFrameManager),
		  descriptorHeap(std::make_unique<DescriptorHeap>(device.CreateDescriptorHeap(EDescriptorHeapType::CBV_SRV_UAV, 1).value()))
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
			commandContexts.emplace_back(std::make_unique<CommandContext>(device.CreateCommandContext("ImGui Cmd Ctx", EQueueType::Direct).value()));
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

		CommandContext& cmdCtx = *commandContexts[frameManager.GetLocalFrameIndex()];
		cmdCtx.Begin();
		Swapchain& swapchain = renderer.GetSwapchain();
		GpuTexture& backBuffer = swapchain.GetBackBuffer();

		const GpuView& backBufferRTV = swapchain.GetRenderTargetView();
		cmdCtx.SetRenderTarget(backBufferRTV);
		cmdCtx.SetDescriptorHeap(*descriptorHeap);

		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), &cmdCtx.GetNative());

		cmdCtx.AddPendingTextureBarrier(backBuffer,
										D3D12_BARRIER_SYNC_RENDER_TARGET, D3D12_BARRIER_SYNC_NONE,
										D3D12_BARRIER_ACCESS_RENDER_TARGET, D3D12_BARRIER_ACCESS_NO_ACCESS,
										D3D12_BARRIER_LAYOUT_RENDER_TARGET, D3D12_BARRIER_LAYOUT_PRESENT);

		cmdCtx.FlushBarriers();
		cmdCtx.End();

		CommandQueue& mainGfxQueue = renderer.GetMainGfxQueue();
		mainGfxQueue.AddPendingContext(cmdCtx);
	}
} // namespace fe