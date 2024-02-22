#include <Engine.h>
#include <Core/InputManager.h>
#include <D3D12/Device.h>
#include <Gameplay/GameInstance.h>
#include <Gameplay/World.h>
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

		std::unique_ptr<fe::World> defaultWorld = std::make_unique<fe::World>();
		const auto				   player = PlayerArchetype::Create(*defaultWorld);
		const auto				   enemy = defaultWorld->Create();

		/* Initialize Buffers */
		constexpr uint16_t NumQuadIndices = 6;
		constexpr uint16_t NumTriIndices = 3;

		dx::GPUBufferDesc quadVBDesc{};
		dx::GPUBufferDesc quadIBDesc{};
		quadVBDesc.AsVertexBuffer<SimpleVertex>(4);
		quadIBDesc.AsIndexBuffer<uint16_t>(NumQuadIndices);
		UniqueHandle<dx::GPUBuffer> quadVB{ handleManager, renderDevice.CreateBuffer(quadVBDesc).value() };
		UniqueHandle<dx::GPUBuffer> quadIB{ handleManager, renderDevice.CreateBuffer(quadIBDesc).value() };

		dx::GPUBufferDesc triVBDesc{};
		dx::GPUBufferDesc triIBDesc{};
		triVBDesc.AsVertexBuffer<SimpleVertex>(3);
		triIBDesc.AsIndexBuffer<uint16_t>(NumTriIndices);
		UniqueHandle<dx::GPUBuffer> triVB{ handleManager, renderDevice.CreateBuffer(triVBDesc).value() };
		UniqueHandle<dx::GPUBuffer> triIB{ handleManager, renderDevice.CreateBuffer(triIBDesc).value() };

		dx::GPUBufferDesc uploadBufferDesc{};
		uploadBufferDesc.AsUploadBuffer(static_cast<uint32_t>(quadVBDesc.GetSizeAsBytes() + quadIBDesc.GetSizeAsBytes() + triVBDesc.GetSizeAsBytes() + triIBDesc.GetSizeAsBytes()));
		dx::GPUBuffer uploadBuffer{ renderDevice.CreateBuffer(uploadBufferDesc).value() };

		const SimpleVertex quadVertices[4] = {
			{ -.75f, 0.f, 0.f },
			{ -.75f, 0.25f, 0.f },
			{ -.50f, 0.25f, 0.f },
			{ -.50f, 0.f, 0.f },
		};
		const uint16_t quadIndices[6] = { 0, 1, 2, 2, 3, 0 };

		const SimpleVertex triVertices[3] = {
			{ 0.5f, 0.f, 0.f },
			{ 0.6f, 0.5f, 0.f },
			{ 0.7f, 0.f, 0.f }
		};
		const uint16_t triIndices[3] = { 0, 1, 2 };

		/* #idea Upload 용 독자적인 CmdCtx와 CmdQueue?, 데이터 한번에 쌓아뒀다가 최대 크기 버퍼 할당후 한번에 업로드 */
		/* Upload data to gpu(upload buffer; cpu->gpu) */
		const size_t quadVerticesOffset = 0;
		const size_t quadVerticesSize = quadVBDesc.GetSizeAsBytes();
		const size_t quadIndicesOffset = quadVerticesSize;
		const size_t quadIndicesSize = quadIBDesc.GetSizeAsBytes();
		const size_t triVerticesOffset = quadIndicesOffset + quadIndicesSize;
		const size_t triVerticesSize = triVBDesc.GetSizeAsBytes();
		const size_t triIndicesOffset = triVerticesOffset + triVerticesSize;
		const size_t triIndicesSize = triIBDesc.GetSizeAsBytes();
		{
			dx::GPUResourceMapGuard mappedBuffer = uploadBuffer.Map();
			std::memcpy(mappedBuffer.get() + quadVerticesOffset, quadVertices, quadVerticesSize);
			std::memcpy(mappedBuffer.get() + quadIndicesOffset, quadIndices, quadIndicesSize);

			std::memcpy(mappedBuffer.get() + triVerticesOffset, triVertices, triVerticesSize);
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
						*quadVB,
						D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COPY,
						D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_COPY_DEST);

					cmdCtx.AddPendingBufferBarrier(
						*quadIB,
						D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COPY,
						D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_COPY_DEST);

					cmdCtx.AddPendingBufferBarrier(
						*quadVB,
						D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COPY,
						D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_COPY_DEST);

					cmdCtx.AddPendingBufferBarrier(
						*quadIB,
						D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COPY,
						D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_COPY_DEST);

					cmdCtx.FlushBarriers();
				}

				/* Copy upload buffer -> each buffer(vb/ib) */
				cmdCtx.CopyBuffer(uploadBuffer, quadVerticesOffset, quadVerticesSize, *quadVB, 0);
				cmdCtx.CopyBuffer(uploadBuffer, quadIndicesOffset, quadIndicesSize, *quadIB, 0);

				cmdCtx.CopyBuffer(uploadBuffer, triVerticesOffset, triVerticesSize, *triVB, 0);
				cmdCtx.CopyBuffer(uploadBuffer, triIndicesOffset, triIndicesSize, *triIB, 0);

				{
					cmdCtx.AddPendingBufferBarrier(
						*quadVB,
						D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_SYNC_VERTEX_SHADING,
						D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_VERTEX_BUFFER);

					cmdCtx.AddPendingBufferBarrier(
						*quadIB,
						D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_SYNC_INDEX_INPUT,
						D3D12_BARRIER_ACCESS_COPY_DEST, D3D12_BARRIER_ACCESS_INDEX_BUFFER);

					cmdCtx.AddPendingBufferBarrier(
						*triVB,
						D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_SYNC_VERTEX_SHADING,
						D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_VERTEX_BUFFER);

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

		const StaticMeshComponent quadStaticMeshComp{
			.VertexBufferHandle = quadVB.DeriveWeak(),
			.IndexBufferHandle = quadIB.DeriveWeak(),
			.NumIndices = NumQuadIndices
		};
		defaultWorld->Attach<StaticMeshComponent>(player, quadStaticMeshComp);

		const StaticMeshComponent triStaticMeshComp{
			.VertexBufferHandle = triVB.DeriveWeak(),
			.IndexBufferHandle = triIB.DeriveWeak(),
			.NumIndices = NumTriIndices
		};
		defaultWorld->Attach<StaticMeshComponent>(enemy, triStaticMeshComp);

		// #wip 이제 PositionComponent 값을 가져와서, Constant Buffer로 데이터 넘겨서, 쉐이더에서 실시간으로 위치 변경해서
		// 렌더링 되도록 만들기

		gameInstance.SetWorld(std::move(defaultWorld));
		gameInstance.SetGameFlow(std::make_unique<BasicGameFlow>());

		fe::ImGuiCanvas& canvas = engine.GetImGuiCanvas();
		canvas.AddLayer<TestLayer>(fe::String("DemoUI"));

		result = engine.Execute();
	}

	return result;
}