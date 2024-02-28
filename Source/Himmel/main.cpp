#include <Engine.h>
#include <Core/InputManager.h>
#include <D3D12/Device.h>
#include <Gameplay/GameInstance.h>
#include <Gameplay/World.h>
#include <Gameplay/PositionComponent.h>
#include <PlayerArchetype.h>
#include <BasicGameFlow.h>
#include <ImGui/ImGuiLayer.h>
#include <ImGui/ImGuiCanvas.h>
#include <Core/Handle.h>
#include <Core/Event.h>
#include <iostream>

// #test Test for imgui integration
class TestLayer : public fe::ImGuiLayer
{
public:
	TestLayer(const fe::String layerName)
		: fe::ImGuiLayer(layerName) {}

	virtual void Render() override
	{
		bool bIsOpend = IsVisible();
		ImGui::ShowDemoWindow(&bIsOpend);
		SetVisibility(bIsOpend);
	}
};

// #test
struct SimpleVertex
{
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;
};

#include <D3D12/GPUBuffer.h>
#include <D3D12/GPUBufferDesc.h>
#include <D3D12/CommandQueue.h>
#include <D3D12/CommandContext.h>
#include <D3D12/Fence.h>
#include <D3D12/GPUViewManager.h>
#include <D3D12/GPUView.h>
#include <Renderer/StaticMeshComponent.h>
#include <Asset/Texture.h>

int main()
{
	using namespace fe;
	int result = 0;
	{
		fe::Engine engine;

		// #test Asset System Test
		TextureImporter importer;
		importer.ImportTexture(String("Resources\\djmax_1st_anv.png"));

		fe::InputManager& inputManager = engine.GetInputManager();
		inputManager.BindAction(fe::String("MoveLeft"), fe::EInput::A);
		inputManager.BindAction(fe::String("MoveRight"), fe::EInput::D);
		inputManager.BindAction(fe::String("UseHealthRecovery"), fe::EInput::Space);
		inputManager.BindAction(fe::String("DisplayPlayerInfo"), fe::EInput::MouseLB);

		fe::dx::Device&		renderDevice = engine.GetRenderDevice();
		fe::GameInstance&	gameInstance = engine.GetGameInstance();
		HandleManager&		handleManager = engine.GetHandleManager();
		dx::GpuViewManager& gpuViewManager = engine.GetGPUViewManager();

		std::unique_ptr<fe::World> defaultWorld = std::make_unique<fe::World>();
		const auto				   player = PlayerArchetype::Create(*defaultWorld);
		const auto				   enemy = defaultWorld->Create();

		/* Initialize Buffers */
		constexpr uint16_t NumQuadIndices = 6;
		constexpr uint16_t NumTriIndices = 3;

		dx::GpuBufferDesc quadIBDesc{};
		quadIBDesc.AsIndexBuffer<uint16_t>(NumQuadIndices);
		quadIBDesc.DebugName = String("QuadMeshIB");
		Handle<dx::GpuBuffer> quadIB{ handleManager, renderDevice.CreateBuffer(quadIBDesc).value() };

		dx::GpuBufferDesc triIBDesc{};
		triIBDesc.AsIndexBuffer<uint16_t>(NumTriIndices);
		triIBDesc.DebugName = String("TriMeshIB");
		Handle<dx::GpuBuffer> triIB{ handleManager, renderDevice.CreateBuffer(triIBDesc).value() };

		dx::GpuBufferDesc verticesBufferDesc{};
		verticesBufferDesc.AsStructuredBuffer<SimpleVertex>(7);
		verticesBufferDesc.DebugName = String("VerticesBuffer.SB");
		Handle<dx::GpuBuffer> verticesBuffer{ handleManager, renderDevice.CreateBuffer(verticesBufferDesc).value() };

		const SimpleVertex vertices[7] = {
			{ -.1f, 0.f, 0.f },
			{ -.1f, 0.5f, 0.f },
			{ .10f, 0.5f, 0.f },
			{ .10f, 0.f, 0.f },
			{ -0.1f, 0.f, 0.f },
			{ 0.0f, 0.45f, 0.f },
			{ 0.1f, 0.f, 0.f }
		};

		dx::GpuBufferDesc uploadBufferDesc{};
		uploadBufferDesc.AsUploadBuffer(static_cast<uint32_t>(sizeof(vertices) + quadIBDesc.GetSizeAsBytes() + triIBDesc.GetSizeAsBytes()));
		dx::GpuBuffer uploadBuffer{ renderDevice.CreateBuffer(uploadBufferDesc).value() };

		const uint16_t quadIndices[6] = { 0, 1, 2, 2, 3, 0 };
		const uint16_t triIndices[3] = { 4, 5, 6 };

		/* #todo Upload 용 독자적인 CmdCtx와 CmdQueue?, 데이터 한번에 쌓아뒀다가 최대 크기 버퍼 할당후 한번에 업로드 */
		/* Upload data to gpu(upload buffer; cpu->gpu) */
		const size_t quadIndicesOffset = sizeof(vertices);
		const size_t quadIndicesSize = quadIBDesc.GetSizeAsBytes();
		const size_t triIndicesOffset = quadIndicesOffset + quadIndicesSize;
		const size_t triIndicesSize = triIBDesc.GetSizeAsBytes();
		{
			dx::GPUResourceMapGuard mappedBuffer = uploadBuffer.MapGuard();
			std::memcpy(mappedBuffer.get() + 0, vertices, sizeof(vertices));
			std::memcpy(mappedBuffer.get() + quadIndicesOffset, quadIndices, quadIndicesSize);
			std::memcpy(mappedBuffer.get() + triIndicesOffset, triIndices, triIndicesSize);
		}

		/* Transfer data from upload buffer to each buffer. (gpu->gpu) & State transfer */
		{
			dx::Fence		   fence = renderDevice.CreateFence("BufferInit").value();
			dx::CommandQueue   directQueue = renderDevice.CreateCommandQueue(fe::dx::EQueueType::Direct).value();
			dx::CommandContext cmdCtx = renderDevice.CreateCommandContext(fe::dx::EQueueType::Direct).value();

			cmdCtx.Begin();
			{
				/* Barriers - NONE -> COPY */
				{
					cmdCtx.AddPendingBufferBarrier(
						uploadBuffer,
						D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COPY,
						D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_COPY_SOURCE);

					cmdCtx.AddPendingBufferBarrier(
						*verticesBuffer,
						D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COPY,
						D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_COPY_DEST);

					cmdCtx.AddPendingBufferBarrier(
						*quadIB,
						D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COPY,
						D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_COPY_DEST);

					cmdCtx.AddPendingBufferBarrier(
						*quadIB,
						D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COPY,
						D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_COPY_DEST);

					cmdCtx.FlushBarriers();
				}

				/* Copy upload buffer -> each buffer(sb/ib) */
				cmdCtx.CopyBuffer(uploadBuffer, 0, sizeof(vertices), *verticesBuffer, 0);

				cmdCtx.CopyBuffer(uploadBuffer, quadIndicesOffset, quadIndicesSize, *quadIB, 0);
				cmdCtx.CopyBuffer(uploadBuffer, triIndicesOffset, triIndicesSize, *triIB, 0);

				{
					cmdCtx.AddPendingBufferBarrier(
						*verticesBuffer,
						D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_SYNC_VERTEX_SHADING,
						D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_SHADER_RESOURCE);

					cmdCtx.AddPendingBufferBarrier(
						*quadIB,
						D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_SYNC_INDEX_INPUT,
						D3D12_BARRIER_ACCESS_COPY_DEST, D3D12_BARRIER_ACCESS_INDEX_BUFFER);

					cmdCtx.AddPendingBufferBarrier(
						*triIB,
						D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_SYNC_INDEX_INPUT,
						D3D12_BARRIER_ACCESS_COPY_DEST, D3D12_BARRIER_ACCESS_INDEX_BUFFER);

					cmdCtx.FlushBarriers();
				}
			}
			cmdCtx.End();

			directQueue.AddPendingContext(cmdCtx);
			directQueue.FlushPendingContexts();
			directQueue.NextSignalTo(fence);
			fence.WaitOnCPU();
		}

		Handle<dx::GpuView, dx::GpuViewManager*> verticesBufferSRV = gpuViewManager.RequestShaderResourceView(*verticesBuffer);

		const StaticMeshComponent quadStaticMeshComp{
			.VerticesBufferSRV = verticesBufferSRV.MakeRef(),
			.IndexBufferHandle = quadIB.MakeRef(),
			.NumIndices = NumQuadIndices
		};
		defaultWorld->Attach<StaticMeshComponent>(player, quadStaticMeshComp);

		const StaticMeshComponent triStaticMeshComp{
			.VerticesBufferSRV = verticesBufferSRV.MakeRef(),
			.IndexBufferHandle = triIB.MakeRef(),
			.NumIndices = NumTriIndices
		};
		defaultWorld->Attach<StaticMeshComponent>(enemy, triStaticMeshComp);
		defaultWorld->Attach<PositionComponent>(enemy, PositionComponent{ .x = .5f, .y = -0.2f });

		gameInstance.SetWorld(std::move(defaultWorld));
		gameInstance.SetGameFlow(std::make_unique<BasicGameFlow>());

		fe::ImGuiCanvas& canvas = engine.GetImGuiCanvas();
		canvas.AddLayer<TestLayer>(fe::String("DemoUI"));

		result = engine.Execute();
	}

	return result;
}

#if defined(CHECK_MEM_LEAK)
	#define _CRTDBG_MAP_ALLOC
	#include <cstdlib>
	#include <crtdbg.h>

struct MemoryLeakDetector
{
	MemoryLeakDetector() { _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); }
	~MemoryLeakDetector() { _CrtDumpMemoryLeaks(); }
};
	#pragma warning(push)
	#pragma warning(disable : 4074)
	#pragma init_seg(compiler)
MemoryLeakDetector memLeakDetector;
	#pragma warning(pop)

#endif
