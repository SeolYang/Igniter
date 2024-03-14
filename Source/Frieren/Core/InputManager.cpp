#include <Core/InputManager.h>
#include <Core/Log.h>
#include <Core/Window.h>
#include <Engine.h>

namespace fe
{
	EInput WParamToInput(const WPARAM wParam)
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

			default:
				return EInput::None;
		}
	}

	InputManager::InputManager(HandleManager& handleManager)
		: handleManager(handleManager)
	{
	}

	void InputManager::BindAction(const String nameOfAction, const EInput input)
	{
		check(nameOfAction);
		check(input != EInput::None);
		check(!inputActionNameMap[input].contains(nameOfAction));
		inputActionNameMap[input].insert(nameOfAction);
		if (!actionMap.contains(nameOfAction))
		{
			actionMap[nameOfAction] = Handle<Action>{ handleManager };
		}
		else
		{
			checkNoEntry();
		}
	}

	void InputManager::BindAxis(const String nameOfAxis, const EInput input, const float scale)
	{
		check(nameOfAxis);
		check(input != EInput::None);
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
			auto			scaleMapItr = scaleMap.find(nameOfAxis);
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
			auto	  scaleMapItr = scaleMap.find(nameOfAxis);
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

			case WM_MOUSEMOVE:
				HandleMouseMove(wParam, lParam);
				break;
		}
	}

	void InputManager::PostUpdate()
	{
		if (!preesedInputSet.empty())
		{
			for (const auto& input : preesedInputSet)
			{
				for (const auto& actionName : inputActionNameMap[input])
				{
					auto& action = actionMap[actionName];
					if (action->State == EInputState::Pressed)
					{
						action->State = EInputState::OnPressing;
					}
				}
			}

			preesedInputSet.clear();
		}
	}

	void InputManager::Clear()
	{
		inputActionNameMap.clear();
		inputAxisNameScaleMap.clear();
		actionMap.clear();
		axisMap.clear();
		preesedInputSet.clear();
	}

	void InputManager::HandleKeyDown(const WPARAM wParam, const LPARAM /** unused */)
	{
		const EInput input = WParamToInput(wParam);
		HandlePressAction(input);
		HandleAxis(input, 1.f);
	}

	void InputManager::HandleKeyUp(const WPARAM wParam, const LPARAM /* unused */)
	{
		const EInput input = WParamToInput(wParam);
		HandleReleaseAction(input);
		HandleAxis(input, 0.f);
	}

	void InputManager::HandleMouseMove(const WPARAM /*wParam*/, const LPARAM lParam)
	{
		constexpr float Infinity = std::numeric_limits<float>::infinity();

		const WindowDescription winDesc = Engine::GetWindow().GetDescription();
		const float				mouseX = static_cast<float>(LOWORD(lParam)) / winDesc.Width;
		const float				mouseY = static_cast<float>(HIWORD(lParam)) / winDesc.Height;
		HandleAxis(EInput::MouseX, mouseX);
		HandleAxis(EInput::MouseY, mouseY);

		if (latestMouseX != Infinity && latestMouseY != Infinity)
		{
			const float relativeMouseX = mouseX - latestMouseX;
			const float relativeMouseY = mouseY - latestMouseY;
			HandleAxis(EInput::RelativeMouseX, relativeMouseX);
			HandleAxis(EInput::RelativeMouseY, relativeMouseY);
		}

		latestMouseX = mouseX;
		latestMouseY = mouseY;
	}

	void InputManager::HandlePressAction(const EInput input)
	{
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
						preesedInputSet.insert(input);
						break;
				}
			}
		}
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

	void InputManager::HandleAxis(const EInput input, const float value)
	{
		auto inputAxisNameScaleMapItr = inputAxisNameScaleMap.find(input);
		if (inputAxisNameScaleMapItr != inputAxisNameScaleMap.end())
		{
			for (const auto& [axisName, axisScale] : inputAxisNameScaleMapItr->second)
			{
				RefHandle<Axis> axis = axisMap[axisName].MakeRef();
				axis->Value = value * axisScale;
			}
		}
	}
} // namespace fe
