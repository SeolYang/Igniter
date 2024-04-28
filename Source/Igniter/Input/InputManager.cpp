#include <Igniter.h>
#include <Core/Log.h>
#include <Input/InputManager.h>

IG_DEFINE_LOG_CATEGORY(InputManager);

namespace ig
{
    namespace
    {
        EInput WParamToInput(const WPARAM wParam, const bool bIsMouseKey)
        {
            if (bIsMouseKey)
            {
                /* #sy_ref https://learn.microsoft.com/ko-kr/windows/win32/learnwin32/mouse-clicks#additional-flags */
                if (ContainsFlags(wParam, MK_LBUTTON))
                {
                    return EInput::MouseLB;
                }

                if (ContainsFlags(wParam, MK_RBUTTON))
                {
                    return EInput::MouseRB;
                }
            }

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

                default:
                    break;
            }

            return EInput::None;
        }
    }    // namespace

    InputManager::InputManager()
    {
        RAWINPUTDEVICE mouseRID{};
        mouseRID.usUsagePage = 0x01; /* HID_USAGE_PAGE_GENERIC */
        mouseRID.usUsage = 0x02;     /* HID_USAGE_GENERIC_MOUSE */
        mouseRID.dwFlags = 0;
        mouseRID.hwndTarget = nullptr;

        if (RegisterRawInputDevices(&mouseRID, 1, sizeof(mouseRID)) == FALSE)
        {
            IG_LOG(InputManager, Fatal, "Failed to create raw input mouse. {:#X}", GetLastError());
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
            IG_LOG(InputManager, Error, "Failed to unregister raw input mouse. {:#X}", GetLastError());
        }

        for (const auto& [_, mappedAction] : nameActionTable)
        {
            actionRegistry.Destroy(mappedAction.ActionHandle);
        }

        for (const auto& [_, mappedAxis] : nameAxisTable)
        {
            axisRegistry.Destroy(mappedAxis.AxisHandle);
        }
    }

    void InputManager::MapAction(const String name, const EInput input)
    {
        if (!name.IsValid() || name.IsEmpty())
        {
            IG_LOG(InputManager, Error, "The Name string has invalid or empty value.");
            return;
        }

        if (input == EInput::None)
        {
            IG_LOG(InputManager, Error, "Action {} cannot mapped with 'None'.", name);
            return;
        }

        if (nameActionTable.contains(name))
        {
            IG_LOG(InputManager, Error, "The Map action ignored due to duplication of {} mapping.");
            return;
        }

        const Handle<Action, InputManager> newHandle = actionRegistry.Create();
        nameActionTable[name] = ActionMapping{.ActionHandle = newHandle, .MappedInput = input};
        IG_CHECK(!actionSets[ToUnderlying(input)].contains(newHandle));
        actionSets[ToUnderlying(input)].insert(newHandle);
        IG_LOG(InputManager, Info, "Action {} mapped to '{}'", name, input);
    }

    void InputManager::UnmapAction(const String name)
    {
        if (!nameActionTable.contains(name))
        {
            IG_LOG(InputManager, Error, "Action {} does not exists.", name);
            return;
        }

        const auto [handle, mappedInput] = nameActionTable[name];
        IG_CHECK(actionSets[ToUnderlying(mappedInput)].contains(handle));
        actionSets[ToUnderlying(mappedInput)].erase(handle);
        actionRegistry.Destroy(handle);
        IG_LOG(InputManager, Info, "Action {} unmapped from '{}'.", name, mappedInput);
    }

    void InputManager::MapAxis(const String name, const EInput input, const float scale)
    {
        if (!name.IsValid() || name.IsEmpty())
        {
            IG_LOG(InputManager, Error, "The Name string has invalid or empty value.");
            return;
        }

        if (input == EInput::None)
        {
            IG_LOG(InputManager, Error, "Axis {} cannot mapped with 'None'.", name);
            return;
        }

        if (nameActionTable.contains(name))
        {
            IG_LOG(InputManager, Error, "The Map axis ignored due to duplication of {} mapping.");
            return;
        }

        const Handle<Axis, InputManager> newHandle = axisRegistry.Create(scale);
        nameAxisTable[name] = AxisMapping{.AxisHandle = newHandle, .MappedInput = input};
        IG_CHECK(!axisSets[ToUnderlying(input)].contains(newHandle));
        axisSets[ToUnderlying(input)].insert(newHandle);
        IG_LOG(InputManager, Info, "Axis {} mapped to '{}'", name, input);
    }

    void InputManager::UnmapAxis(const String name)
    {
        if (!nameAxisTable.contains(name))
        {
            IG_LOG(InputManager, Error, "Axis {} does not exists.", name);
            return;
        }

        const auto [handle, mappedInput] = nameAxisTable[name];
        IG_CHECK(axisSets[ToUnderlying(mappedInput)].contains(handle));
        axisSets[ToUnderlying(mappedInput)].erase(handle);
        axisRegistry.Destroy(handle);
        IG_LOG(InputManager, Info, "Axis {} unmapped from '{}'.", name, mappedInput);
    }

    void InputManager::SetScale(const String name, const float newScale)
    {
        if (!nameAxisTable.contains(name))
        {
            IG_LOG(InputManager, Error, "Axis {} does not exists.", name);
            return;
        }

        Axis* axisPtr = axisRegistry.Lookup(nameAxisTable[name].AxisHandle);
        IG_CHECK(axisPtr != nullptr);
        axisPtr->Scale = newScale;
    }

    Handle<Action, InputManager> InputManager::QueryAction(const String name) const
    {
        const auto mappingItr = nameActionTable.find(name);
        if (mappingItr != nameActionTable.cend())
        {
            return mappingItr->second.ActionHandle;
        }

        IG_LOG(InputManager, Error, "Action {} does not exists.", name);
        return Handle<Action, InputManager>{};
    }

    Handle<Axis, InputManager> InputManager::QueryAxis(const String name) const
    {
        const auto mappingItr = nameAxisTable.find(name);
        if (mappingItr != nameAxisTable.cend())
        {
            return mappingItr->second.AxisHandle;
        }

        IG_LOG(InputManager, Error, "Axis {} does not exists.", name);
        return Handle<Axis, InputManager>{};
    }

    Action InputManager::GetAction(const Handle<Action, InputManager> action) const
    {
        const Action* actionPtr = actionRegistry.Lookup(action);
        if (actionPtr == nullptr)
        {
            IG_LOG(InputManager, Error, "The action handle is not valid.");
            return Action{};
        }

        return *actionPtr;
    }

    Axis InputManager::GetAxis(const Handle<Axis, InputManager> axis) const
    {
        const Axis* axisPtr = axisRegistry.Lookup(axis);
        if (axisPtr == nullptr)
        {
            IG_LOG(InputManager, Error, "The action handle is not valid.");
            return Axis{};
        }

        return *axisPtr;
    }

    void InputManager::HandleEvent(const uint32_t message, const WPARAM wParam, const LPARAM lParam)
    {
        constexpr WPARAM mouseLBWParam = 1;
        constexpr WPARAM mouseRBWParam = 2;

        switch (message)
        {
            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
                HandleKeyDown(wParam, true);
                break;

            case WM_KEYDOWN:
                HandleKeyDown(wParam, false);
                break;

            case WM_LBUTTONUP:
                HandleKeyUp(mouseLBWParam, true);
                break;
            case WM_RBUTTONUP:
                HandleKeyUp(mouseRBWParam, true);
                break;

            case WM_KEYUP:
                HandleKeyUp(wParam, false);
                break;

            case WM_INPUT:
                HandleRawInput(lParam);
                break;

            default:
                break;
        }
    }

    void InputManager::PostUpdate()
    {
        for (const EInput input : processedInputs)
        {
            for (const Handle<Action, InputManager> actionHandle : actionSets[ToUnderlying(input)])
            {
                Action* actionPtr = actionRegistry.Lookup(actionHandle);
                IG_CHECK(actionPtr != nullptr);
                switch (actionPtr->State)
                {
                    case EInputState::Pressed:
                        actionPtr->State = EInputState::OnPressing;
                        break;

                    case EInputState::Released:
                        actionPtr->State = EInputState::None;
                        break;

                    default:
                        break;
                }
            }

            for (const Handle<Axis, InputManager> axisHandle : axisSets[ToUnderlying(input)])
            {
                Axis* axisPtr = axisRegistry.Lookup(axisHandle);
                IG_CHECK(axisPtr != nullptr);
                axisPtr->Value = 0.f;
            }
        }

        processedInputs.clear();
    }

    void InputManager::HandleKeyDown(const WPARAM wParam, const bool bIsMouse)
    {
        const EInput input = WParamToInput(wParam, bIsMouse);
        if (HandlePressAction(input) || HandleAxis(input, 1.f))
        {
            processedInputs.insert(input);
        }
    }

    void InputManager::HandleKeyUp(const WPARAM wParam, const bool bIsMouse)
    {
        const EInput input = WParamToInput(wParam, bIsMouse);
        HandleReleaseAction(input);
        HandleAxis(input, 0.f);
    }

    void InputManager::HandleRawInput(const LPARAM lParam)
    {
        UINT pcbSize{};
        GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &pcbSize, sizeof(RAWINPUTHEADER));
        if (rawInputBuffer.size() < pcbSize)
        {
            rawInputBuffer.resize(pcbSize);
        }

        if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, rawInputBuffer.data(), &pcbSize, sizeof(RAWINPUTHEADER)) != pcbSize)
        {
            IG_LOG(InputManager, Fatal, "GetRawInputData does not return correct size!");
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
                processedInputs.insert(EInput::MouseDeltaX);
            }

            if (HandleAxis(EInput::MouseDeltaY, static_cast<float>(deltaY), true))
            {
                processedInputs.insert(EInput::MouseDeltaY);
            }
        }
    }

    bool InputManager::HandlePressAction(const EInput input)
    {
        bool bAnyPress = false;
        for (const Handle<Action, InputManager> action : actionSets[ToUnderlying(input)])
        {
            Action* actionPtr = actionRegistry.Lookup(action);
            IG_CHECK(actionPtr != nullptr);
            switch (actionPtr->State)
            {
                case EInputState::None:
                case EInputState::Released:
                    actionPtr->State = EInputState::Pressed;
                    bAnyPress = true;
                    break;
                default:
                    break;
            }
        }

        return bAnyPress;
    }

    bool InputManager::HandleReleaseAction(const EInput input)
    {
        for (const Handle<Action, InputManager> action : actionSets[ToUnderlying(input)])
        {
            Action* actionPtr = actionRegistry.Lookup(action);
            IG_CHECK(actionPtr != nullptr);
            actionPtr->State = EInputState::Released;
        }

        return false;
    }

    bool InputManager::HandleAxis(const EInput input, const float value, const bool bIsDifferential)
    {
        bool bAnyAxisHandled = false;
        for (const Handle<Axis, InputManager> axis : axisSets[ToUnderlying(input)])
        {
            Axis* axisPtr = axisRegistry.Lookup(axis);
            IG_CHECK(axisPtr != nullptr);
            if (bIsDifferential)
            {
                axisPtr->Value += value;
            }
            else
            {
                axisPtr->Value = value;
            }

            bAnyAxisHandled = true;
        }

        return bAnyAxisHandled;
    }
}    // namespace ig
