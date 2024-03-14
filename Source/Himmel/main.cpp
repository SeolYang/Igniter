#include <Engine.h>
#include <Core/InputManager.h>
#include <Core/Handle.h>
#include <Core/Event.h>
#include <Core/Window.h>
#include <Gameplay/GameInstance.h>
#include <Gameplay/World.h>
#include <D3D12/RenderDevice.h>
#include <D3D12/GPUBuffer.h>
#include <D3D12/GPUBufferDesc.h>
#include <D3D12/CommandQueue.h>
#include <D3D12/CommandContext.h>
#include <D3D12/GPUView.h>
#include <Render/GPUViewManager.h>
#include <Render/GpuUploader.h>
#include <Render/StaticMeshComponent.h>
#include <Render/TransformComponent.h>
#include <Render/CameraArchetype.h>
#include <Asset/Texture.h>
#include <Asset/Model.h>
#include <ImGui/ImGuiLayer.h>
#include <ImGui/ImGuiCanvas.h>
#include <PlayerArchetype.h>
#include <EnemyArchetype.h>
#include <BasicGameFlow.h>
#include <MenuBar.h>
#include <iostream>
#include <future>

// #sy_test Test for imgui integration
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

// #sy_test
struct SimpleVertex
{
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;
	float u = 0.f;
	float v = 0.f;
};

int main()
{
	using namespace fe;
	int result = 0;
	{
		Engine engine;
		Window& window = engine.GetWindow();
		InputManager& inputManager = engine.GetInputManager();
		RenderDevice& renderDevice = engine.GetRenderDevice();
		GameInstance& gameInstance = engine.GetGameInstance();
		HandleManager& handleManager = engine.GetHandleManager();
		GpuViewManager& gpuViewManager = engine.GetGPUViewManager();
		GpuUploader& gpuUploader = engine.GetGpuUploader();

		/* #sy_test Asset System & Mechanism Test */
		/* #sy_todo Check thread-safety of TextureImporter.Import */
		TextureImporter texImporter;
		// TextureImportConfig texImportConfig = MakeVersionedDefault<TextureImportConfig>();
		// texImportConfig.bGenerateMips = true;
		// texImportConfig.CompressionMode = fe::ETextureCompressionMode::BC7;
		// texImporter.Import(String("Resources\\Ash\\ash.jpg"), texImportConfig);
		// texImporter.Import(String("Resources\\Homura\\homura.jpg"), texImportConfig);

		// StaticMeshImportConfig staticMeshImportConfig = MakeVersionedDefault<StaticMeshImportConfig>();
		//staticMeshImportConfig.bMergeMeshes = true;
		//std::vector<xg::Guid> staticMeshAssetGuids = ModelImporter::ImportAsStatic(texImporter, String("Resources\\Ash\\Ash.fbx"), staticMeshImportConfig);

		const xg::Guid ashStaticMeshGuid = xg::Guid("5fe88685-41be-450a-8cfc-3aa074f774c0");
		//std::optional<StaticMesh> staticMesh = StaticMeshLoader::Load(ashStaticMeshGuid, handleManager, renderDevice, gpuUploader, gpuViewManager);
		//check(staticMesh);

		const xg::Guid ashThumbnailGuid = xg::Guid("512a7388-949d-4c30-9c21-5c08e2dfa587");
		const xg::Guid homuraThumbnailGuid = xg::Guid("0aa4c1e2-41fa-429c-959b-4abfbfef2a08");

		std::future<std::optional<StaticMesh>> ashStaticMeshFuture = std::async(
			std::launch::async,
			StaticMeshLoader::Load,
			ashStaticMeshGuid,
			std::ref(handleManager), std::ref(renderDevice), std::ref(gpuUploader), std::ref(gpuViewManager));

		std::future<std::optional<Texture>> ashThumbnailFutre = std::async(
			std::launch::async,
			TextureLoader::Load,
			ashThumbnailGuid,
			std::ref(handleManager), std::ref(renderDevice), std::ref(gpuUploader), std::ref(gpuViewManager));

		std::future<std::optional<Texture>> homuraThumbnailFuture = std::async(
			std::launch::async,
			TextureLoader::Load,
			homuraThumbnailGuid,
			std::ref(handleManager), std::ref(renderDevice), std::ref(gpuUploader), std::ref(gpuViewManager));

		/******************************/

		/* #sy_test Input Manager Test */
		inputManager.BindAction(fe::String("MoveLeft"), fe::EInput::A);
		inputManager.BindAction(fe::String("MoveRight"), fe::EInput::D);
		inputManager.BindAction(fe::String("UseHealthRecovery"), fe::EInput::Space);
		inputManager.BindAction(fe::String("DisplayPlayerInfo"), fe::EInput::MouseLB);
		/********************************/

		/* #sy_test ECS based Game flow & logic tests */
		std::unique_ptr<fe::World> defaultWorld = std::make_unique<fe::World>();

		auto ashStaticMesh = ashStaticMeshFuture.get();
		check(ashStaticMesh);
		auto ashThumbnail = ashThumbnailFutre.get();
		check(ashThumbnail);
		const StaticMeshComponent quadStaticMeshComp{
			.VerticesBufferSRV = ashStaticMesh->VertexBufferSrv.MakeRef(),
			.IndexBufferHandle = ashStaticMesh->IndexBufferInstance.MakeRef(),
			.NumIndices = ashStaticMesh->LoadConfig.NumIndices,
			.DiffuseTex = ashThumbnail->ShaderResourceView.MakeRef(),
			.DiffuseTexSampler = ashThumbnail->TexSampler
		};

		const auto player = PlayerArchetype::Create(*defaultWorld);
		defaultWorld->Attach<StaticMeshComponent>(player, quadStaticMeshComp);
		TransformComponent& playerTransform = defaultWorld->Get<TransformComponent>(player);
		playerTransform.Scale = Vector3{ 10.f, 10.f, 0.f };

		auto homuraThumbnail = homuraThumbnailFuture.get();
		check(homuraThumbnail);
		const StaticMeshComponent triStaticMeshComp{
			.VerticesBufferSRV = ashStaticMesh->VertexBufferSrv.MakeRef(),
			.IndexBufferHandle = ashStaticMesh->IndexBufferInstance.MakeRef(),
			.NumIndices = ashStaticMesh->LoadConfig.NumIndices,
			.DiffuseTex = homuraThumbnail->ShaderResourceView.MakeRef(),
			.DiffuseTexSampler = homuraThumbnail->TexSampler
		};

		const auto enemy = EnemyArchetype::Create(*defaultWorld);
		defaultWorld->Attach<StaticMeshComponent>(enemy, triStaticMeshComp);
		TransformComponent& enemyTransform = defaultWorld->Get<TransformComponent>(enemy);
		enemyTransform.Position = Vector3{ 10.5f, -3.5f, 10.f };
		enemyTransform.Scale = Vector3{ 50.f, 50.f, 1.f };

		const Entity cameraEntity = CameraArchetype::Create(*defaultWorld);
		Camera& camera = defaultWorld->Get<Camera>(cameraEntity);
		camera.CameraViewport = window.GetViewport();
		TransformComponent& cameraTransform = defaultWorld->Get<TransformComponent>(cameraEntity);
		cameraTransform.Position = Vector3{ 0.f, 0.f, -10.f };

		gameInstance.SetWorld(std::move(defaultWorld));
		gameInstance.SetGameFlow(std::make_unique<BasicGameFlow>());
		/********************************************/

		/* #sy_test ImGui integration tests */
		fe::ImGuiCanvas& canvas = engine.GetImGuiCanvas();
		canvas.AddLayer<TestLayer>(fe::String("DemoUI"));
		canvas.AddLayer<MenuBar>();
		/************************************/

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
