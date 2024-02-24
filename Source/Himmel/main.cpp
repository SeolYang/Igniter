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

int main()
{
	using namespace fe;
	int result = 0;
	{
		fe::Engine		  engine;
		fe::InputManager& inputManager = engine.GetInputManager();
		inputManager.BindAction(fe::String("MoveLeft"), fe::EInput::A);
		inputManager.BindAction(fe::String("MoveRight"), fe::EInput::D);
		inputManager.BindAction(fe::String("UseHealthRecovery"), fe::EInput::Space);
		inputManager.BindAction(fe::String("DisplayPlayerInfo"), fe::EInput::MouseLB);

		fe::dx::Device&	  renderDevice = engine.GetRenderDevice();
		fe::GameInstance& gameInstance = engine.GetGameInstance();
		HandleManager&	  handleManager = engine.GetHandleManager();
		dx::GPUViewManager& gpuViewManager = engine.GetGPUViewManager();

		std::unique_ptr<fe::World> defaultWorld = std::make_unique<fe::World>();
		const auto				   player = PlayerArchetype::Create(*defaultWorld);
		const auto				   enemy = defaultWorld->Create();

		/* Initialize Buffers */
		constexpr uint16_t NumQuadIndices = 6;
		constexpr uint16_t NumTriIndices = 3;

		dx::GPUBufferDesc quadIBDesc{};
		quadIBDesc.AsIndexBuffer<uint16_t>(NumQuadIndices);
		UniqueHandle<dx::GPUBuffer> quadIB{ handleManager, renderDevice.CreateBuffer(quadIBDesc).value() };

		dx::GPUBufferDesc triIBDesc{};
		triIBDesc.AsIndexBuffer<uint16_t>(NumTriIndices);
		UniqueHandle<dx::GPUBuffer> triIB{ handleManager, renderDevice.CreateBuffer(triIBDesc).value() };

		dx::GPUBufferDesc verticesBufferDesc{};
		verticesBufferDesc.AsStructuredBuffer<SimpleVertex>(7);
		UniqueHandle<dx::GPUBuffer> verticesBuffer{ handleManager, renderDevice.CreateBuffer(verticesBufferDesc).value() };

		const SimpleVertex vertices[7] = {
			{ -.1f, 0.f, 0.f },
			{ -.1f, 0.5f, 0.f },
			{ .10f, 0.5f, 0.f },
			{ .10f, 0.f, 0.f },
			{ -0.1f, 0.f, 0.f },
			{ 0.0f, 0.45f, 0.f },
			{ 0.1f, 0.f, 0.f }
		};

		dx::GPUBufferDesc uploadBufferDesc{};
		uploadBufferDesc.AsUploadBuffer(static_cast<uint32_t>(sizeof(vertices) + quadIBDesc.GetSizeAsBytes() + triIBDesc.GetSizeAsBytes()));
		dx::GPUBuffer uploadBuffer{ renderDevice.CreateBuffer(uploadBufferDesc).value() };

		const uint16_t quadIndices[6] = { 0, 1, 2, 2, 3, 0 };
		const uint16_t triIndices[3] = { 4, 5, 6 };

		/* #idea Upload 용 독자적인 CmdCtx와 CmdQueue?, 데이터 한번에 쌓아뒀다가 최대 크기 버퍼 할당후 한번에 업로드 */
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

		UniqueHandle<dx::GPUView, dx::GPUViewManager*> verticesBufferSRV = gpuViewManager.RequestShaderResourceView(*verticesBuffer);

		const StaticMeshComponent quadStaticMeshComp{
			.VerticesBufferSRV = verticesBufferSRV.DeriveWeak(),
			.IndexBufferHandle = quadIB.DeriveWeak(),
			.NumIndices = NumQuadIndices
		};
		defaultWorld->Attach<StaticMeshComponent>(player, quadStaticMeshComp);

		const StaticMeshComponent triStaticMeshComp{
			.VerticesBufferSRV = verticesBufferSRV.DeriveWeak(),
			.IndexBufferHandle = triIB.DeriveWeak(),
			.NumIndices = NumTriIndices
		};
		defaultWorld->Attach<StaticMeshComponent>(enemy, triStaticMeshComp);
		defaultWorld->Attach<PositionComponent>(enemy);

		gameInstance.SetWorld(std::move(defaultWorld));
		gameInstance.SetGameFlow(std::make_unique<BasicGameFlow>());

		fe::ImGuiCanvas& canvas = engine.GetImGuiCanvas();
		canvas.AddLayer<TestLayer>(fe::String("DemoUI"));

		result = engine.Execute();
	}

	return result;
}