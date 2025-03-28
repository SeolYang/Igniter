#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/String.h"
#include "Igniter/Core/HandleStorage.h"

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
        [[nodiscard]] F32 GetScaledValue() const noexcept { return Value * Scale; }

    public:
        F32 Scale = 1.f;
        F32 Value = 0.0f;
    };

    class InputManager final
    {
    private:
        struct ActionMapping
        {
            Handle<Action> ActionHandle{};
            EInput MappedInput{EInput::None};
        };

        struct AxisMapping
        {
            Handle<Axis> AxisHandle{};
            EInput MappedInput{EInput::None};
        };

        struct RawMouseInput
        {
            F32 DeltaX = 0.f;
            F32 DeltaY = 0.f;
        };

    public:
        InputManager();
        InputManager(const InputManager&) = delete;
        InputManager(InputManager&&) noexcept = delete;
        ~InputManager();

        InputManager& operator=(const InputManager&) = delete;
        InputManager& operator=(InputManager&&) noexcept = delete;

        void MapAction(const std::string& name, const EInput input);
        void UnmapAction(const std::string& name);

        void MapAxis(const std::string& name, const EInput input, const F32 scale = 1.f);
        void UnmapAxis(const std::string& name);
        void SetScale(const std::string& name, const F32 newScale);

        [[nodiscard]] Handle<Action> QueryAction(const std::string& name) const;
        [[nodiscard]] Handle<Axis> QueryAxis(const std::string& name) const;

        [[nodiscard]] Action GetAction(const Handle<Action> action) const;
        [[nodiscard]] Axis GetAxis(const Handle<Axis> axis) const;

        bool HandleEvent(const U32 message, const WPARAM wParam, const LPARAM lParam);

        void HandleRawMouseInput();
        void PostUpdate();

    private:
        void HandleKeyDown(const WPARAM wParam, const bool bIsMouse);
        void HandleKeyUp(const WPARAM wParam, const bool bIsMouse);

        bool HandlePressAction(const EInput input);
        bool HandleReleaseAction(const EInput input);

        bool HandleAxis(const EInput input, const F32 value, const bool bIsDifferential = false);

        void PollRawMouseInput();

    private:
        HandleStorage<Action> actionRegistry;
        HandleStorage<Axis> axisRegistry;

        /* @todo https://github.com/martinus/unordered_dense?tab=readme-ov-file#324-heterogeneous-overloads-using-is_transparent 를 활용하여 임시 std::string 객체 생성 최소화 하기 */
        UnorderedMap<std::string, ActionMapping> nameActionTable{};
        UnorderedMap<std::string, AxisMapping> nameAxisTable{};

        constexpr static Size kNumScopedInputs = magic_enum::enum_count<EInput>();
        eastl::array<UnorderedSet<Handle<Action>>, kNumScopedInputs> actionSets;
        eastl::array<UnorderedSet<Handle<Axis>>, kNumScopedInputs> axisSets;

        UnorderedSet<EInput> processedInputs;

        std::jthread rawMouseInputPollingThread;
        HANDLE rawMouseInputPollingDoneEvent{INVALID_HANDLE_VALUE};
        Mutex rawMouseInputPollingMutex;
        eastl::vector<uint8_t> rawInputBuffer;
        eastl::vector<RawMouseInput> polledRawMouseInputs;
        Size rawInputOffset{sizeof(RAWINPUTHEADER)};
    };
} // namespace ig
