#include <Core/InputManager.h>
#include <Core/Log.h>
#include <Core/Window.h>
#include <Core/TypeUtils.h>
#include <Core/Igniter.h>

namespace ig
{
    IG_DEFINE_LOG_CATEGORY(InputManagerInfo, ELogVerbosity::Info)
    IG_DEFINE_LOG_CATEGORY(InputManagerDebug, ELogVerbosity::Debug)
    IG_DEFINE_LOG_CATEGORY(InputManagerError, ELogVerbosity::Error)

    static EInput WParamToInput(const WPARAM wParam)
    {
        switch (wParam)
        {
            /** Characters */
            case 'W':
            case 'w':
                return EInput::W;

            case 'S':
            case 's':
                return EInput::S;

            case 'A':
            case 'a':
                return EInput::A;

            case 'D':
            case 'd':
                return EInput::D;

            /** Virtual Keys */
            case VK_SPACE:
                return EInput::Space;

            case VK_LBUTTON:
                return EInput::MouseLB;

            case VK_RBUTTON:
                return EInput::MouseRB;

            case VK_SHIFT:
                return EInput::Shift;

            case VK_CONTROL:
                return EInput::Control;
        }

        /* #sy_ref https://learn.microsoft.com/ko-kr/windows/win32/learnwin32/mouse-clicks#additional-flags */
        if (BitFlagContains(wParam, MK_LBUTTON))
        {
            return EInput::MouseLB;
        }

        if (BitFlagContains(wParam, MK_RBUTTON))
        {
            return EInput::MouseRB;
        }

        return EInput::None;
    }

    InputManager::InputManager(HandleManager& handleManager)
        : handleManager(handleManager)
    {
        RAWINPUTDEVICE mouseRID{};
        mouseRID.usUsagePage = 0x01; /* HID_USAGE_PAGE_GENERIC */
        mouseRID.usUsage = 0x02;     /* HID_USAGE_GENERIC_MOUSE */
        mouseRID.dwFlags = 0;
        mouseRID.hwndTarget = nullptr;

        if (RegisterRawInputDevices(&mouseRID, 1, sizeof(mouseRID)) == FALSE)
        {
            IG_LOG(InputManagerError, "Failed to create raw input mouse. {:#X}", GetLastError());
        }
    }

    InputManager::~InputManager()
    {
        RAWINPUTDEVICE mouseRID{};
        mouseRID.usUsagePage = 0x01; /* HID_USAGE_PAGE_GENERIC */
        mouseRID.usUsage = 0x02;     /* HID_USAGE_GENERIC_MOUSE */
        mouseRID.dwFlags = RIDEV_REMOVE;
        mouseRID.hwndTarget = nullptr;

        if (RegisterRawInputDevices(&mouseRID, 1, sizeof(mouseRID)) == FALSE)
        {
            IG_LOG(InputManagerError, "Failed to unregister raw input mouse. {:#X}", GetLastError());
        }
    }

    void InputManager::BindAction(const String nameOfAction, const EInput input)
    {
        IG_CHECK(nameOfAction);
        IG_CHECK(input != EInput::None);
        IG_CHECK(!inputActionNameMap[input].contains(nameOfAction));
        inputActionNameMap[input].insert(nameOfAction);
        if (!actionMap.contains(nameOfAction))
        {
            actionMap[nameOfAction] = Handle<Action>{ handleManager };
        }
    }

    void InputManager::BindAxis(const String nameOfAxis, const EInput input, const float scale)
    {
        IG_CHECK(nameOfAxis);
        IG_CHECK(input != EInput::None);
        if (!axisMap.contains(nameOfAxis))
        {
            inputAxisNameScaleMap[input][nameOfAxis] = scale;
            axisMap[nameOfAxis] = Handle<Axis>{ handleManager };
        }
    }

    RefHandle<const Action> InputManager::QueryAction(const String nameOfAction) const
    {
        auto actionMapItr = actionMap.find(nameOfAction);
        return actionMapItr != actionMap.cend() ? actionMapItr->second.MakeRef() : RefHandle<const Action>{};
    }

    RefHandle<const Axis> InputManager::QueryAxis(const String nameOfAxis) const
    {
        auto axisMapItr = axisMap.find(nameOfAxis);
        return axisMapItr != axisMap.cend() ? axisMapItr->second.MakeRef() : RefHandle<const Axis>{};
    }

    float InputManager::QueryScaleOfAxis(const String nameOfAxis, const EInput input) const
    {
        auto itr = inputAxisNameScaleMap.find(input);
        if (itr != inputAxisNameScaleMap.cend())
        {
            const ScaleMap& scaleMap = itr->second;
            auto scaleMapItr = scaleMap.find(nameOfAxis);
            if (scaleMapItr != scaleMap.cend())
            {
                return scaleMapItr->second;
            }
        }

        return 0.f;
    }

    void InputManager::SetScaleOfAxis(const String nameOfAxis, const EInput input, const float scale)
    {
        auto itr = inputAxisNameScaleMap.find(input);
        if (itr != inputAxisNameScaleMap.end())
        {
            ScaleMap& scaleMap = itr->second;
            auto scaleMapItr = scaleMap.find(nameOfAxis);
            if (scaleMapItr != scaleMap.end())
            {
                scaleMapItr->second = scale;
            }
        }
    }

    void InputManager::HandleEvent(const UINT message, const WPARAM wParam, const LPARAM lParam)
    {
        constexpr WPARAM MouseLBWParam = 1;
        constexpr WPARAM MouseRBWParam = 2;

        switch (message)
        {
            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_KEYDOWN:
                HandleKeyDown(wParam, lParam);
                break;

            case WM_LBUTTONUP:
                HandleKeyUp(MouseLBWParam, lParam);
                break;
            case WM_RBUTTONUP:
                HandleKeyUp(MouseRBWParam, lParam);
                break;

            case WM_KEYUP:
                HandleKeyUp(wParam, lParam);
                break;

            case WM_INPUT:
                HandleRawInputDevices(wParam, lParam);
                break;
        }
    }

    void InputManager::PostUpdate()
    {
        if (!scopedInputs.empty())
        {
            for (const auto& input : scopedInputs)
            {
                for (const auto& actionName : inputActionNameMap[input])
                {
                    const auto actionItr = actionMap.find(actionName);
                    if (actionItr != actionMap.end())
                    {
                        auto& action = actionItr->second;
                        IG_CHECK(action);
                        if (action->State == EInputState::Pressed)
                        {
                            action->State = EInputState::OnPressing;
                        }
                    }
                    else
                    {
                        IG_CHECK_NO_ENTRY();
                    }
                }

                const auto axisNameScaleMapItr = inputAxisNameScaleMap.find(input);
                if (axisNameScaleMapItr != inputAxisNameScaleMap.end())
                {
                    for (auto& axisNameScale : axisNameScaleMapItr->second)
                    {
                        const auto axisItr = axisMap.find(axisNameScale.first);
                        if (axisItr != axisMap.end())
                        {
                            auto& axis = axisItr->second;
                            IG_CHECK(axis);
                            axis->Value = 0.f;
                        }
                    }
                }
            }

            scopedInputs.clear();
        }
    }

    void InputManager::Clear()
    {
        inputActionNameMap.clear();
        inputAxisNameScaleMap.clear();
        actionMap.clear();
        axisMap.clear();
        scopedInputs.clear();
    }

    void InputManager::HandleKeyDown(const WPARAM wParam, const LPARAM /** unused */)
    {
        const EInput input = WParamToInput(wParam);
        const bool bKeyDownHandled = HandlePressAction(input) || HandleAxis(input, 1.f);
        if (bKeyDownHandled)
        {
            scopedInputs.insert(input);
        }
    }

    void InputManager::HandleKeyUp(const WPARAM wParam, const LPARAM /* unused */)
    {
        const EInput input = WParamToInput(wParam);
        HandleReleaseAction(input);
        HandleAxis(input, 0.f);
    }

    void InputManager::HandleRawInputDevices(const WPARAM, const LPARAM lParam)
    {
        UINT pcbSize{};
        GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &pcbSize, sizeof(RAWINPUTHEADER));
        if (rawInputBuffer.size() < pcbSize)
        {
            rawInputBuffer.resize(pcbSize);
        }

        if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, rawInputBuffer.data(), &pcbSize, sizeof(RAWINPUTHEADER)) != pcbSize)
        {
            IG_LOG(InputManagerError, "GetRawInputData does not return correct size!");
            return;
        }

        const auto* rawInput = reinterpret_cast<RAWINPUT*>(rawInputBuffer.data());
        IG_CHECK(rawInput != nullptr);
        if (rawInput->header.dwType == RIM_TYPEMOUSE)
        {
            const auto deltaX = rawInput->data.mouse.lLastX;
            const auto deltaY = rawInput->data.mouse.lLastY;
            if (HandleAxis(EInput::MouseDeltaX, static_cast<float>(deltaX), true))
            {
                scopedInputs.insert(EInput::MouseDeltaX);
            }

            if (HandleAxis(EInput::MouseDeltaY, static_cast<float>(deltaY), true))
            {
                scopedInputs.insert(EInput::MouseDeltaY);
            }
        }
    }

    bool InputManager::HandlePressAction(const EInput input)
    {
        bool bPressActionHandled = false;
        auto inputActionNameItr = inputActionNameMap.find(input);
        if (inputActionNameItr != inputActionNameMap.end())
        {
            for (const String& actionName : inputActionNameItr->second)
            {
                RefHandle<Action> action = actionMap[actionName].MakeRef();
                switch (action->State)
                {
                    case EInputState::None:
                    case EInputState::Released:
                        action->State = EInputState::Pressed;
                        bPressActionHandled = true;
                }
            }
        }

        return bPressActionHandled;
    }

    void InputManager::HandleReleaseAction(const EInput input)
    {
        auto inputActionNameItr = inputActionNameMap.find(input);
        if (inputActionNameItr != inputActionNameMap.end())
        {
            for (const String& actionName : inputActionNameItr->second)
            {
                RefHandle<Action> action = actionMap[actionName].MakeRef();
                action->State = EInputState::None;
            }
        }
    }

    bool InputManager::HandleAxis(const EInput input, const float value, const bool bIsDifferential)
    {
        bool bAxisHandled = false;
        auto inputAxisNameScaleMapItr = inputAxisNameScaleMap.find(input);
        if (inputAxisNameScaleMapItr != inputAxisNameScaleMap.end())
        {
            for (const auto& [axisName, axisScale] : inputAxisNameScaleMapItr->second)
            {
                RefHandle<Axis> axis = axisMap[axisName].MakeRef();
                if (bIsDifferential)
                {
                    axis->Value += value * axisScale;
                }
                else
                {
                    axis->Value = value * axisScale;
                }

                bAxisHandled = true;
            }
        }

        return bAxisHandled;
    }
} // namespace ig
