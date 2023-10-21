#include <Core/InputManager.h>
#include <Core/Engine.h>
#include <Core/Log.h>

namespace fe
{
	FE_DECLARE_LOG_CATEGORY(LogInputInfo, ELogVerbosiy::Info);

	void InputManager::BindAction(const String nameOfAction, const EInput input)
	{
		const bool bIsValidName = nameOfAction;
		const bool bIsValidInput = input != EInput::None;
		FE_ASSERT(bIsValidName);
		FE_ASSERT(bIsValidInput);
		if (bIsValidName && bIsValidInput)
		{
			inputActionNameMap[input].insert(nameOfAction);
			if (!actionMap.contains(nameOfAction))
			{
				actionMap[nameOfAction] = OwnedHandle<Action>::Create(Engine::GetHandleManager(), nameOfAction);
			}
		}
	}

	void InputManager::BindAxis(const String nameOfAxis, const EInput input, const float scale)
	{
		const bool bIsValidName = nameOfAxis;
		const bool bIsValidInput = input != EInput::None;
		FE_ASSERT(bIsValidName);
		FE_ASSERT(bIsValidInput);
		if (bIsValidName && bIsValidInput)
		{
			if (!axisMap.contains(nameOfAxis))
			{
				inputAxisNameScaleMap[input][nameOfAxis] = scale;
				axisMap[nameOfAxis] = OwnedHandle<Axis>::Create(Engine::GetHandleManager(), nameOfAxis);
			}
		}
	}

	WeakHandle<const Action> InputManager::QueryAction(const String nameOfAction) const
	{
		auto actionMapItr = actionMap.find(nameOfAction);
		return actionMapItr != actionMap.cend() ? actionMapItr->second.DeriveWeak() : WeakHandle<const Action>{};
	}

	WeakHandle<const Axis> InputManager::QueryAxis(const String nameOfAxis) const
	{
		auto axisMapItr = axisMap.find(nameOfAxis);
		return axisMapItr != axisMap.cend() ? axisMapItr->second.DeriveWeak() : WeakHandle<const Axis>{};
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
		switch (message)
		{
			case WM_KEYDOWN:
				HandleKeyDown(wParam, lParam);
				break;

			case WM_KEYUP:
				HandleKeyUp(wParam, lParam);
				break;

			case WM_MOUSEMOVE:
				HandleMouseMove(wParam, lParam);
				break;
		}
	}

	void InputManager::HandleKeyDown(const WPARAM wParam, const LPARAM /** unused */)
	{
		const EInput input = Private::WParamToInput(wParam);
		HandlePressAction(input);
		HandleAxis(input, 1.f);
	}

	void InputManager::HandleKeyUp(const WPARAM wParam, const LPARAM /* unused */)
	{
		const EInput input = Private::WParamToInput(wParam);
		HandleReleaseAction(input);
		HandleAxis(input, 0.f);
	}

	void InputManager::HandleMouseMove(WPARAM wParam, LPARAM lParam)
	{
		/* @todo impl mouse move capture*/
	}

	void InputManager::HandlePressAction(const EInput input)
	{
		auto inputActionNameItr = inputActionNameMap.find(input);
		if (inputActionNameItr != inputActionNameMap.end())
		{
			for (const String& actionName : inputActionNameItr->second)
			{
				WeakHandle<Action> action = actionMap[actionName].DeriveWeak();
				switch (action->State)
				{
					case EInputState::None:
					case EInputState::Released:
						action->State = EInputState::Pressed;
						break;

					case EInputState::Pressed:
						action->State = EInputState::OnPressing;
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
				WeakHandle<Action> action = actionMap[actionName].DeriveWeak();
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
				WeakHandle<Axis> axis = axisMap[axisName];
				axis->Value = value * axisScale;
			}
		}
	}
} // namespace fe
