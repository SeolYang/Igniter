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
#include <CameraController.h>
#include <BasicGameFlow.h>
#include <MenuBar.h>
#include <iostream>
#include <future>

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

		/* #sy_test Window class tests */
		window.ClipCursor();
		/*******************************/

		/* #sy_test Asset System & Mechanism Test */
		/* #sy_todo Check thread-safety of TextureImporter.Import */
		TextureImporter texImporter;
		// TextureImportConfig texImportConfig = MakeVersionedDefault<TextureImportConfig>();
		// texImportConfig.bGenerateMips = true;
		// texImportConfig.CompressionMode = ETextureCompressionMode::BC7;
		// texImporter.Import(String("Resources\\Homura\\Body.png"), texImportConfig);

		StaticMeshImportConfig staticMeshImportConfig = MakeVersionedDefault<StaticMeshImportConfig>();
		// staticMeshImportConfig.bMergeMeshes = true;
		// std::vector<xg::Guid> staticMeshAssetGuids = ModelImporter::ImportAsStatic(texImporter, String("Resources\\Homura\\Axe.fbx"), staticMeshImportConfig);

		const xg::Guid ashBodyTexGuid = xg::Guid("4b2b2556-7d81-4884-ba50-777392ebc9ee");
		const xg::Guid ashThumbnailGuid = xg::Guid("512a7388-949d-4c30-9c21-5c08e2dfa587");
		const xg::Guid ashStaticMeshGuid = xg::Guid("e42f93b4-e57d-44ae-9fde-302013c6e9e8");

		const xg::Guid homuraThumbnailGuid = xg::Guid("0aa4c1e2-41fa-429c-959b-4abfbfef2a08");
		const xg::Guid homuraBodyTexGuid = xg::Guid("87949751-3431-45c7-bd57-0a1518649511");
		const xg::Guid homuraStaticMeshGuid = xg::Guid("a717ff6e-129b-4c80-927a-a786a0b21128");

		const xg::Guid axeTexGuid = xg::Guid("eb93c3b0-1197-4884-8cfe-622f66184d4c");
		const xg::Guid axeStaticMeshGuid = xg::Guid("4452fc34-9c3f-4715-8b6c-4481ab16b9bb");

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

		std::future<std::optional<Texture>> ashBodyTexFuture = std::async(
			std::launch::async,
			TextureLoader::Load,
			ashBodyTexGuid,
			std::ref(handleManager), std::ref(renderDevice), std::ref(gpuUploader), std::ref(gpuViewManager));

		std::future<std::optional<StaticMesh>> ashStaticMeshFuture = std::async(
			std::launch::async,
			StaticMeshLoader::Load,
			ashStaticMeshGuid,
			std::ref(handleManager), std::ref(renderDevice), std::ref(gpuUploader), std::ref(gpuViewManager));

		std::future<std::optional<Texture>> axeTexFutrue = std::async(
			std::launch::async,
			TextureLoader::Load,
			axeTexGuid,
			std::ref(handleManager), std::ref(renderDevice), std::ref(gpuUploader), std::ref(gpuViewManager));

		std::future<std::optional<StaticMesh>> axeStaticMeshFuture = std::async(
			std::launch::async,
			StaticMeshLoader::Load,
			axeStaticMeshGuid,
			std::ref(handleManager), std::ref(renderDevice), std::ref(gpuUploader), std::ref(gpuViewManager));

		std::future<std::optional<StaticMesh>> homuraStaticMeshFuture = std::async(
			std::launch::async,
			StaticMeshLoader::Load,
			homuraStaticMeshGuid,
			std::ref(handleManager), std::ref(renderDevice), std::ref(gpuUploader), std::ref(gpuViewManager));

		std::future<std::optional<Texture>> homuraBodyTexFuture = std::async(
			std::launch::async,
			TextureLoader::Load,
			homuraBodyTexGuid,
			std::ref(handleManager), std::ref(renderDevice), std::ref(gpuUploader), std::ref(gpuViewManager));

		/******************************/

		/* #sy_test Input Manager Test */
		inputManager.BindAction(String("MoveLeft"), EInput::A);
		inputManager.BindAction(String("MoveRight"), EInput::D);
		inputManager.BindAction(String("MoveForward"), EInput::W);
		inputManager.BindAction(String("MoveBackward"), EInput::S);
		inputManager.BindAction(String("MoveUp"), EInput::MouseLB);
		inputManager.BindAction(String("MoveDown"), EInput::MouseRB);
		inputManager.BindAction(String("Sprint"), EInput::Shift);

		inputManager.BindAxis(String("TurnYaw"), EInput::MouseDeltaX, 1.f);
		inputManager.BindAxis(String("TurnAxis"), EInput::MouseDeltaY, 1.f);

		/********************************/

		auto homuraThumbnail = homuraThumbnailFuture.get();
		check(homuraThumbnail);
		auto ashThumbnail = ashThumbnailFutre.get();
		check(ashThumbnail);

		/* #sy_test ECS based Game flow & logic tests */
		std::unique_ptr<World> defaultWorld = std::make_unique<World>();

		/* Player Entity */
		const auto player = PlayerArchetype::Create(*defaultWorld);
		auto homuraBodyTex = homuraBodyTexFuture.get();
		check(homuraBodyTex);
		auto homuraStaticMesh = homuraStaticMeshFuture.get();
		check(homuraStaticMesh);
		{
			StaticMeshComponent& playerStaticMeshComponent = defaultWorld->Attach<StaticMeshComponent>(player);
			playerStaticMeshComponent.VerticesBufferSRV = homuraStaticMesh->VertexBufferSrv.MakeRef();
			playerStaticMeshComponent.IndexBufferHandle = homuraStaticMesh->IndexBufferInstance.MakeRef();
			playerStaticMeshComponent.NumIndices = homuraStaticMesh->LoadConfig.NumIndices;
			playerStaticMeshComponent.DiffuseTex = homuraBodyTex->ShaderResourceView.MakeRef();
			playerStaticMeshComponent.DiffuseTexSampler = homuraBodyTex->TexSampler;

			TransformComponent& playerTransform = defaultWorld->Get<TransformComponent>(player);
			playerTransform.Position = Vector3{ 10.5f, -30.f, 10.f };
			playerTransform.Scale = Vector3{ 0.35f, 0.35f, 0.35f };
		}

		/* Enemy Entity */
		const auto enemy = EnemyArchetype::Create(*defaultWorld);
		auto ashBodyTex = ashBodyTexFuture.get();
		check(ashBodyTex);
		auto ashStaticMesh = ashStaticMeshFuture.get();
		check(ashStaticMesh);
		{
			StaticMeshComponent& enemeyStaticMeshComponent = defaultWorld->Attach<StaticMeshComponent>(enemy);
			enemeyStaticMeshComponent.VerticesBufferSRV = ashStaticMesh->VertexBufferSrv.MakeRef();
			enemeyStaticMeshComponent.IndexBufferHandle = ashStaticMesh->IndexBufferInstance.MakeRef();
			enemeyStaticMeshComponent.NumIndices = ashStaticMesh->LoadConfig.NumIndices;
			enemeyStaticMeshComponent.DiffuseTex = ashBodyTex->ShaderResourceView.MakeRef();
			enemeyStaticMeshComponent.DiffuseTexSampler = ashBodyTex->TexSampler;

			TransformComponent& enemyTransform = defaultWorld->Get<TransformComponent>(enemy);
			enemyTransform.Position = Vector3{ -10.f, -30.f, 35.f };
			enemyTransform.Scale = Vector3{ 40.f, 40.f, 40.f };
		}

		/* Axe Entity */
		const Entity axeEntity = defaultWorld->Create();
		auto axeTex = axeTexFutrue.get();
		check(axeTex);
		auto axeStaticMesh = axeStaticMeshFuture.get();
		check(axeStaticMesh);
		{
			StaticMeshComponent& axeStaticMeshComponent = defaultWorld->Attach<StaticMeshComponent>(axeEntity);
			axeStaticMeshComponent.VerticesBufferSRV = axeStaticMesh->VertexBufferSrv.MakeRef();
			axeStaticMeshComponent.IndexBufferHandle = axeStaticMesh->IndexBufferInstance.MakeRef();
			axeStaticMeshComponent.NumIndices = axeStaticMesh->LoadConfig.NumIndices;
			axeStaticMeshComponent.DiffuseTex = axeTex->ShaderResourceView.MakeRef();
			axeStaticMeshComponent.DiffuseTexSampler = axeTex->TexSampler;

			TransformComponent& axeTransformComponent = defaultWorld->Attach<TransformComponent>(axeEntity);
			axeTransformComponent.Position = Vector3{ -15.f, 0.f, 2.0f };
			axeTransformComponent.Scale = Vector3{
				3.f, 3.f, 3.f
			};
		}

		/* Camera */
		const Entity cameraEntity = CameraArchetype::Create(*defaultWorld);
		{
			Camera& camera = defaultWorld->Get<Camera>(cameraEntity);
			camera.CameraViewport = window.GetViewport();
			TransformComponent& cameraTransform = defaultWorld->Get<TransformComponent>(cameraEntity);
			cameraTransform.Position = Vector3{ 0.f, 0.f, -30.f };
		}

		gameInstance.SetWorld(std::move(defaultWorld));
		gameInstance.SetGameFlow(std::make_unique<BasicGameFlow>());
		/********************************************/

		/* #sy_test ImGui integration tests */
		ImGuiCanvas& canvas = engine.GetImGuiCanvas();
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
