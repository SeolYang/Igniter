#include <Engine.h>
#include <Core/InputManager.h>
#include <D3D12/RenderDevice.h>
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
	float u = 0.f;
	float v = 0.f;
};

#include <D3D12/GPUBuffer.h>
#include <D3D12/GPUBufferDesc.h>
#include <D3D12/CommandQueue.h>
#include <D3D12/CommandContext.h>
#include <D3D12/GPUView.h>
#include <Asset/Texture.h>
#include <Render/GPUViewManager.h>
#include <Render/GpuUploader.h>
#include <Render/StaticMeshComponent.h>

int main()
{
	using namespace fe;
	int result = 0;
	{
		fe::Engine engine;

		// #test Asset System Test
		//TextureImporter importer;
		//xg::Guid importedTexGuid = *importer.Import(String("Resources\\djmax_1st_anv.png"));

		fe::InputManager& inputManager = engine.GetInputManager();
		inputManager.BindAction(fe::String("MoveLeft"), fe::EInput::A);
		inputManager.BindAction(fe::String("MoveRight"), fe::EInput::D);
		inputManager.BindAction(fe::String("UseHealthRecovery"), fe::EInput::Space);
		inputManager.BindAction(fe::String("DisplayPlayerInfo"), fe::EInput::MouseLB);

		fe::RenderDevice& renderDevice = engine.GetRenderDevice();
		fe::GameInstance& gameInstance = engine.GetGameInstance();
		HandleManager& handleManager = engine.GetHandleManager();
		GpuViewManager& gpuViewManager = engine.GetGPUViewManager();
		GpuUploader& gpuUploader = engine.GetGpuUploader();

		std::unique_ptr<fe::World> defaultWorld = std::make_unique<fe::World>();
		const auto player = PlayerArchetype::Create(*defaultWorld);
		const auto enemy = defaultWorld->Create();

		std::optional<Texture> texture = TextureLoader::Load(
			xg::Guid("97a69908-99a9-493a-9485-510254ec10c3"),
			handleManager, renderDevice, gpuUploader, gpuViewManager);

		/* Initialize Buffers */
		constexpr uint16_t NumQuadIndices = 6;
		constexpr uint16_t NumTriIndices = 3;
		const uint16_t quadIndices[NumQuadIndices] = { 0, 1, 2, 2, 3, 0 };
		const uint16_t triIndices[NumTriIndices] = { 4, 5, 6 };

		GpuBufferDesc quadIBDesc{};
		quadIBDesc.AsIndexBuffer<uint16_t>(NumQuadIndices);
		quadIBDesc.DebugName = String("QuadMeshIB");
		Handle<GpuBuffer> quadIB{ handleManager, renderDevice.CreateBuffer(quadIBDesc).value() };

		GpuBufferDesc triIBDesc{};
		triIBDesc.AsIndexBuffer<uint16_t>(NumTriIndices);
		triIBDesc.DebugName = String("TriMeshIB");
		Handle<GpuBuffer> triIB{ handleManager, renderDevice.CreateBuffer(triIBDesc).value() };

		const SimpleVertex vertices[7] = {
			{ -.1f, 0.f, 0.f, /*uv*/ 0.f, 1.f },
			{ -.1f, 0.5f, 0.f, /*uv*/ 0.f, 0.f },
			{ .10f, 0.5f, 0.f, /*uv*/ 1.f, 0.f },
			{ .10f, 0.f, 0.f, /*uv*/ 1.f, 1.f },

			{ -0.1f, 0.f, 0.f, /*uv*/ 0.f, 1.f },
			{ 0.0f, 0.45f, 0.f, /*uv*/ 0.5f, 0.f },
			{ 0.1f, 0.f, 0.f, /*uv*/ 1.f, 1.f }
		};
		GpuBufferDesc verticesBufferDesc{};
		verticesBufferDesc.AsStructuredBuffer<SimpleVertex>(7);
		verticesBufferDesc.DebugName = String("VerticesBuffer.SB");
		Handle<GpuBuffer> verticesBuffer{ handleManager, renderDevice.CreateBuffer(verticesBufferDesc).value() };

		/* GPU Uploader Test */
		std::thread t0(
			[&gpuUploader, &quadIndices, &quadIB]()
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(Random(1, 100)));
				UploadContext quadIndicesUploadCtx = gpuUploader.Reserve(sizeof(quadIndices));
				quadIndicesUploadCtx.WriteData(
					reinterpret_cast<const uint8_t*>(quadIndices),
					0, 0, sizeof(quadIndices));

				quadIndicesUploadCtx->CopyBuffer(
					quadIndicesUploadCtx.GetBuffer(),
					quadIndicesUploadCtx.GetOffset(), sizeof(quadIndices),
					*quadIB, 0);

				std::optional<GpuSync> quadIndicesUploadSync = gpuUploader.Submit(quadIndicesUploadCtx);
				check(quadIndicesUploadSync);
				quadIndicesUploadSync->WaitOnCpu();
			});

		std::thread t1(
			[&gpuUploader, &triIndices, &triIB]()
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(Random(1, 100)));
				UploadContext triIndicesUploadCtx = gpuUploader.Reserve(sizeof(triIndices));
				{
					triIndicesUploadCtx.WriteData(
						reinterpret_cast<const uint8_t*>(triIndices),
						0, 0, sizeof(triIndices));

					triIndicesUploadCtx->CopyBuffer(
						triIndicesUploadCtx.GetBuffer(),
						triIndicesUploadCtx.GetOffset(), sizeof(triIndices),
						*triIB, 0);
				}
				std::optional<GpuSync> triIndicesUploadSync = gpuUploader.Submit(triIndicesUploadCtx);
				check(triIndicesUploadSync);
				triIndicesUploadSync->WaitOnCpu();
			});

		std::thread t2(
			[&gpuUploader, &vertices, &verticesBuffer]()
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(Random(1, 100)));
				UploadContext verticesUploadCtx = gpuUploader.Reserve(sizeof(vertices));
				{
					verticesUploadCtx.WriteData(
						reinterpret_cast<const uint8_t*>(vertices),
						0, 0, sizeof(vertices));

					verticesUploadCtx->CopyBuffer(
						verticesUploadCtx.GetBuffer(),
						verticesUploadCtx.GetOffset(), sizeof(vertices),
						*verticesBuffer, 0);
				}
				std::optional<GpuSync> verticesUploadSync = gpuUploader.Submit(verticesUploadCtx);
				check(verticesUploadSync);
				verticesUploadSync->WaitOnCpu();
			});

		t0.join();
		t1.join();
		t2.join();

		Handle<GpuView, GpuViewManager*> verticesBufferSRV = gpuViewManager.RequestShaderResourceView(*verticesBuffer);

		const StaticMeshComponent quadStaticMeshComp{
			.VerticesBufferSRV = verticesBufferSRV.MakeRef(),
			.IndexBufferHandle = quadIB.MakeRef(),
			.NumIndices = NumQuadIndices,
			.DiffuseTex = texture->FullRangeSrv.MakeRef(),
			.DiffuseTexSampler = texture->TexSampler
		};
		defaultWorld->Attach<StaticMeshComponent>(player, quadStaticMeshComp);

		const StaticMeshComponent triStaticMeshComp{
			.VerticesBufferSRV = verticesBufferSRV.MakeRef(),
			.IndexBufferHandle = triIB.MakeRef(),
			.NumIndices = NumTriIndices,
			.DiffuseTex = texture->FullRangeSrv.MakeRef(),
			.DiffuseTexSampler = texture->TexSampler
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
