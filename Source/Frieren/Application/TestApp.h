#pragma once
#include "Igniter/Application/Application.h"

namespace ig
{
    class World;
    class GameSystem;
} // namespace ig

namespace fe
{
    class Renderer;
    class TestGameSystem;
    class EditorCanvas;

    class TestApp : public ig::Application
    {
    public:
        explicit TestApp(const ig::AppDesc& desc);
        ~TestApp() override;

        void Update(const float deltaTime) override;

        void OnImGui() override;

        void SetGameSystem(ig::Ptr<ig::GameSystem> newGameSystem);

    private:
        ig::Ptr<ig::GameSystem> gameSystem;
        ig::Ptr<EditorCanvas> editorCanvas;

#if defined(DEBUG) || defined(_DEBUG)
        constexpr static ig::U32 kAxeGridSizeX = 1;
        constexpr static ig::U32 kAxeGridSizeY = 1;
        constexpr static ig::U32 kAxeGridSizeZ = 100;
#else
        constexpr static ig::U32 kAxeGridSizeX = 1;
        constexpr static ig::U32 kAxeGridSizeY = 10000;
        constexpr static ig::U32 kAxeGridSizeZ = 10;
#endif

        constexpr static ig::U32 kNumAxes = kAxeGridSizeX * kAxeGridSizeY * kAxeGridSizeZ;
        constexpr static ig::F32 kAxeSpaceInterval = 3.5f;
        constexpr static ig::Vector3 kAxeOffset{-kAxeSpaceInterval * kAxeGridSizeX / 2.f, -kAxeSpaceInterval* kAxeGridSizeY / 2.f, -kAxeSpaceInterval* kAxeGridSizeZ / 2.f};
        constexpr static ig::Vector3 kAxeMinBound{-kAxeSpaceInterval * kAxeGridSizeX / 2.f, -kAxeSpaceInterval* kAxeGridSizeY / 2.f, -kAxeSpaceInterval* kAxeGridSizeZ / 2.f};
        constexpr static ig::Vector3 kAxeMaxBound{kAxeSpaceInterval * kAxeGridSizeX / 2.f, kAxeSpaceInterval* kAxeGridSizeY / 2.f, kAxeSpaceInterval* kAxeGridSizeZ / 2.f};
    };
} // namespace fe
