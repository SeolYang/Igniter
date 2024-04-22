#pragma once
#include <Igniter.h>
#include <Core/String.h>
#include <Core/HandleRegistry.h>

namespace ig
{
    enum class EInput : uint16_t
    {
        None,

        /** Keyboard inputs */
        W,
        A,
        S,
        D,
        Space,
        Shift,
        Control,

        /** Mouse inputs */
        MouseDeltaX,
        MouseDeltaY,
        MouseLB,
        MouseRB
    };

    enum class EInputState
    {
        None,
        Pressed,
        OnPressing,
        Released,
    };

    struct Action final
    {
    public:
        [[nodiscard]] bool IsAnyPressing() const noexcept
        {
            return State == ig::EInputState::Pressed || State == EInputState::OnPressing;
        }

    public:
        EInputState State = EInputState::None;
    };

    struct Axis final
    {
    public:
        [[nodiscard]] float GetScaledValue() const noexcept { return Value * Scale; }

    public:
        float Scale = 1.f;
        float Value = 0.0f;
    };

    class InputManager final
    {
    private:
        struct ActionMapping
        {
            Handle_New<Action> ActionHandle{};
            EInput             MappedInput{EInput::None};
        };

        struct AxisMapping
        {
            Handle_New<Axis> AxisHandle{};
            EInput           MappedInput{EInput::None};
        };

    public:
        InputManager();
        InputManager(const InputManager&)     = delete;
        InputManager(InputManager&&) noexcept = delete;
        ~InputManager();

        InputManager& operator=(const InputManager&)     = delete;
        InputManager& operator=(InputManager&&) noexcept = delete;

        void MapAction(const String name, const EInput input);
        void UnmapAction(const String name);

        void MapAxis(const String name, const EInput input, const float scale = 1.f);
        void UnmapAxis(const String name);
        void SetScale(const String name, const float newScale);

        [[nodiscard]] Handle_New<Action> QueryAction(const String name) const;
        [[nodiscard]] Handle_New<Axis>   QueryAxis(const String name) const;

        [[nodiscard]] Action GetAction(const Handle_New<Action> action) const;
        [[nodiscard]] Axis   GetAxis(const Handle_New<Axis> axis) const;

        void HandleEvent(const uint32_t message, const WPARAM wParam, const LPARAM lParam);
        void PostUpdate();

    private:
        void HandleKeyDown(const WPARAM wParam, const bool bIsMouse);
        void HandleKeyUp(const WPARAM wParam, const bool bIsMouse);
        void HandleRawInput(const LPARAM lParam);

        bool HandlePressAction(const EInput input);
        bool HandleReleaseAction(const EInput input);

        bool HandleAxis(const EInput input, const float value, const bool bIsDifferential = false);

    private:
        HandleRegistry<Action> actionRegistry;
        HandleRegistry<Axis>   axisRegistry;

        UnorderedMap<String, ActionMapping> nameActionTable{};
        UnorderedMap<String, AxisMapping>   nameAxisTable{};

        constexpr static size_t NumScopedInputs = magic_enum::enum_count<EInput>();
        std::array<UnorderedSet<Handle_New<Action>>, NumScopedInputs> actionSets;
        std::array<UnorderedSet<Handle_New<Axis>>, NumScopedInputs> axisSets;

        std::vector<EInput> processedInputs;

        std::vector<uint8_t> rawInputBuffer;
    };
}
