#pragma once
#include <Igniter.h>
#include <Core/String.h>
#include <Core/Handle.h>

namespace ig
{
    enum class EInput
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
        bool IsAnyPressing() const { return State == ig::EInputState::Pressed || State == EInputState::OnPressing; }

    public:
        EInputState State = EInputState::None;

    private:
        float pad = 0.f;
    };

    struct Axis final
    {
    public:
        float Value = 0.0f;

    private:
        float pad = 0.f;
    };

    class HandleManager;

    class InputManager final
    {
        using NameSet           = UnorderedSet<String>;
        using InputNameMap      = UnorderedMap<EInput, NameSet>;
        using ScaleMap          = UnorderedMap<String, float>;
        using InputNameScaleMap = UnorderedMap<EInput, ScaleMap>;
        template <typename T>
        using EventMap = UnorderedMap<String, Handle<T>>;

    public:
        InputManager(HandleManager& handleManager);
        InputManager(const InputManager&)     = delete;
        InputManager(InputManager&&) noexcept = delete;
        ~InputManager();

        InputManager& operator=(const InputManager&)     = delete;
        InputManager& operator=(InputManager&&) noexcept = delete;

        void BindAction(String nameOfAction, EInput input);
        void BindAxis(String nameOfAxis, EInput input, const float scale = 1.f);

        // #sy_todo UnbindAction/Axis

        RefHandle<const Action> QueryAction(String nameOfAction) const;
        RefHandle<const Axis>   QueryAxis(String nameOfAxis) const;
        float                   QueryScaleOfAxis(String nameOfAxis, EInput input) const;

        void SetScaleOfAxis(String nameOfAxis, EInput input, float scale);

        void HandleEvent(UINT message, WPARAM wParam, LPARAM lParam);
        void PostUpdate();

        void Clear();

    private:
        void HandleKeyDown(WPARAM wParam, const bool bIsMouseKey);
        void HandleKeyUp(WPARAM wParam, const bool bIsMouseKey);
        void HandleRawInputDevices(const WPARAM wParam, const LPARAM lParam);

        bool HandlePressAction(EInput input);
        void HandleReleaseAction(EInput input);
        bool HandleAxis(EInput input, float value, const bool bIsDifferential = false);

    private:
        HandleManager& handleManager;

        InputNameMap      inputActionNameMap{};
        InputNameScaleMap inputAxisNameScaleMap{};

        EventMap<Action> actionMap{};
        EventMap<Axis>   axisMap{};

        UnorderedSet<EInput> scopedInputs{};

        std::vector<uint8_t> rawInputBuffer;
    };
} // namespace ig
