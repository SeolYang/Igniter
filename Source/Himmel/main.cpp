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

int main()
{
	// #todo #log 2024/02/19 10:48, GPUView 전부 FrameHandle화
	int result = 0;
	{
		fe::Engine engine;

		auto& hm = engine.GetHandleManager();

		constexpr size_t Iters = 10000000;
		std::vector<fe::UniqueHandle<uint64_t>> handles;
		handles.reserve(Iters);

		namespace chrono = std::chrono;
		auto begin = chrono::high_resolution_clock::now();
		for (size_t idx = 0; idx < Iters; ++idx)
		{
			handles.emplace_back(hm, rand());
		}

		auto end = chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - begin);
		std::cout << end.count() << std::endl;

		fe::InputManager& inputManager = engine.GetInputManager();
		inputManager.BindAction(fe::String("MoveLeft"), fe::EInput::A);
		inputManager.BindAction(fe::String("MoveRight"), fe::EInput::D);
		inputManager.BindAction(fe::String("UseHealthRecovery"), fe::EInput::Space);
		inputManager.BindAction(fe::String("DisplayPlayerInfo"), fe::EInput::MouseLB);

		fe::GameInstance&		   gameInstance = engine.GetGameInstance();
		std::unique_ptr<fe::World> defaultWorld = std::make_unique<fe::World>();
		PlayerArchetype::Create(*defaultWorld);
		gameInstance.SetWorld(std::move(defaultWorld));
		gameInstance.SetGameFlow(std::make_unique<BasicGameFlow>());

		fe::ImGuiCanvas& canvas = engine.GetImGuiCanvas();
		canvas.AddLayer<TestLayer>(fe::String("DemoUI"));

		result = engine.Execute();
	}

	return result;
}