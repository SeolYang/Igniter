#pragma once
#include <Application/Application.h>

namespace ig
{
    class World;
    class GameMode;
}    // namespace ig

namespace fe
{
    class TestGameMode;
    class EditorCanvas;
    class TestApp : public ig::Application
    {
    public:
        TestApp(const AppDesc& desc);
        ~TestApp() override;

        void Update() override;
        void Render() override;

        void SetGameMode(Ptr<ig::GameMode> newGameMode);

        ig::World* GetActiveWorld() { return world.get(); }

    private:
        Ptr<ig::World> world;
        Ptr<ig::GameMode> gameMode;
        Ptr<EditorCanvas> editorCanvas;
    };
}    // namespace fe