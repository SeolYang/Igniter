#pragma once
#include <Core/String.h>
#include <Core/HandleManager.h>
#include <Core/Win32API.h>

namespace fe
{
	enum class EInput
	{
		None,

		/** Keyboard physical inputs */
		W,
		A,
		S,
		D,
		Space,

		/** Mouse physical inputs */
		MouseX,
		MouseY,
		RelativeMouseX,
		RelativeMouseY,
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

	struct Action
	{
		EInputState State = EInputState::None;
	};

	struct Axis
	{
		float Value = 0.0f;
	};

	// @dependency	fe::Engine, fe::Logger, fe::HandleManager, fe::Window
	class InputManager
	{
	public:
		InputManager() = default;
		InputManager(const InputManager&) = delete;
		InputManager(InputManager&&) noexcept = delete;

		InputManager& operator=(const InputManager&) = delete;
		InputManager& operator=(InputManager&&) noexcept = delete;

		// @todo HOW TO HANDLE MULTIPLE INPUTS? -> Average of input?, priority? LIFO?
		void BindAction(String nameOfAction, EInput input);
		void BindAxis(String nameOfAxis, EInput input, float scale);

		// @todo should i release the input binding?
		// void ReleaseAction(String nameOfAction)...

		// @todo should i release input from action?
		// void ReleaseInputFromAction(String nameOfAction, EInput key)...

		WeakHandle<const Action> QueryAction(String nameOfAction) const;
		WeakHandle<const Axis>	 QueryAxis(String nameOfAxis) const;
		float					 QueryScaleOfAxis(String nameOfAxis, EInput input) const;

		void SetScaleOfAxis(String nameOfAxis, EInput input, float scale);

		void HandleEvent(UINT message, WPARAM wParam, LPARAM lParam);

	private:
		void HandleKeyDown(WPARAM wParam, LPARAM lParam);
		void HandleKeyUp(WPARAM wParam, LPARAM lParam);
		void HandleMouseMove(WPARAM wParam, LPARAM lParam);

		void HandlePressAction(EInput input);
		void HandleReleaseAction(EInput input);
		void HandleAxis(EInput input, float value);

	private:
		using NameSet = robin_hood::unordered_set<String>;
		using InputNameMap = robin_hood::unordered_map<EInput, NameSet>;
		using ScaleMap = robin_hood::unordered_map<String, float>;
		using InputNameScaleMap = robin_hood::unordered_map<EInput, ScaleMap>;
		template <typename T>
		using EventMap = robin_hood::unordered_map<String, OwnedHandle<T>>;

		InputNameMap	  inputActionNameMap;
		InputNameScaleMap inputAxisNameScaleMap;

		EventMap<Action> actionMap;
		EventMap<Axis>	 axisMap;

		float latestMouseX = std::numeric_limits<float>::infinity();
		float latestMouseY = std::numeric_limits<float>::infinity();
	};

	namespace Private
	{
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
				default:
					return EInput::None;
			}
		}
	} // namespace Private
} // namespace fe
