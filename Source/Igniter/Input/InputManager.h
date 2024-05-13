#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/String.h"
#include "Igniter/Core/HandleRegistry.h"

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
        [[nodiscard]] bool IsAnyPressing() const noexcept { return State == ig::EInputState::Pressed || State == EInputState::OnPressing; }

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
            Handle<Action, InputManager> ActionHandle{};
            EInput MappedInput{EInput::None};
        };

        struct AxisMapping
        {
            Handle<Axis, InputManager> AxisHandle{};
            EInput MappedInput{EInput::None};
        };

        struct RawMouseInput
        {
            float DeltaX = 0.f;
            float DeltaY = 0.f;
        };

    public:
        InputManager();
        InputManager(const InputManager&) = delete;
        InputManager(InputManager&&) noexcept = delete;
        ~InputManager();

        InputManager& operator=(const InputManager&) = delete;
        InputManager& operator=(InputManager&&) noexcept = delete;

        void MapAction(const String name, const EInput input);
        void UnmapAction(const String name);

        void MapAxis(const String name, const EInput input, const float scale = 1.f);
        void UnmapAxis(const String name);
        void SetScale(const String name, const float newScale);

        [[nodiscard]] Handle<Action, InputManager> QueryAction(const String name) const;
        [[nodiscard]] Handle<Axis, InputManager> QueryAxis(const String name) const;

        [[nodiscard]] Action GetAction(const Handle<Action, InputManager> action) const;
        [[nodiscard]] Axis GetAxis(const Handle<Axis, InputManager> axis) const;

        void HandleEvent(const uint32_t message, const WPARAM wParam, const LPARAM lParam);

        void HandleRawMouseInput();
        void PostUpdate();

    private:
        void HandleKeyDown(const WPARAM wParam, const bool bIsMouse);
        void HandleKeyUp(const WPARAM wParam, const bool bIsMouse);

        bool HandlePressAction(const EInput input);
        bool HandleReleaseAction(const EInput input);

        bool HandleAxis(const EInput input, const float value, const bool bIsDifferential = false);

        void PollRawMouseInput();

    private:
        HandleRegistry<Action, InputManager> actionRegistry;
        HandleRegistry<Axis, InputManager> axisRegistry;

        UnorderedMap<String, ActionMapping> nameActionTable{};
        UnorderedMap<String, AxisMapping> nameAxisTable{};

        constexpr static size_t NumScopedInputs = magic_enum::enum_count<EInput>();
        eastl::array<UnorderedSet<Handle<Action, InputManager>>, NumScopedInputs> actionSets;
        eastl::array<UnorderedSet<Handle<Axis, InputManager>>, NumScopedInputs> axisSets;

        UnorderedSet<EInput> processedInputs;


        std::jthread rawMouseInputPollingThread;
        HANDLE rawMouseInputPollingDoneEvent{INVALID_HANDLE_VALUE};
        Mutex rawMouseInputPollingMutex;
        eastl::vector<uint8_t> rawInputBuffer;
        eastl::vector<RawMouseInput> polledRawMouseInputs;
        size_t rawInputOffset{sizeof(RAWINPUTHEADER)};
    };
}    // namespace ig
