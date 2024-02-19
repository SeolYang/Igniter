#pragma once
#include <Core/String.h>
#include <Core/Win32API.h>
#include <Core/Handle.h>

namespace fe
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

		/** Mouse inputs */
		MouseX,
		MouseY,
		RelativeMouseX,
		RelativeMouseY,
		MouseLB,
		MouseRB
	};

	EInput WParamToInput(const WPARAM wParam);

	enum class EInputState
	{
		None,
		Pressed,
		OnPressing,
		Released,
	};

	struct Action
	{
	public:
		bool IsAnyPressing() const { return State == fe::EInputState::Pressed || State == EInputState::OnPressing; }

	public:
		EInputState State = EInputState::None;
	};

	struct Axis
	{
		float Value = 0.0f;
	};

	class HandleManager;
	class InputManager
	{
		using NameSet = robin_hood::unordered_set<String>;
		using InputNameMap = robin_hood::unordered_map<EInput, NameSet>;
		using ScaleMap = robin_hood::unordered_map<String, float>;
		using InputNameScaleMap = robin_hood::unordered_map<EInput, ScaleMap>;
		template <typename T>
		using EventMap = robin_hood::unordered_map<String, UniqueHandle<T>>;

	public:
		InputManager(HandleManager& handleManager);
		InputManager(const InputManager&) = delete;
		InputManager(InputManager&&) noexcept = delete;

		InputManager& operator=(const InputManager&) = delete;
		InputManager& operator=(InputManager&&) noexcept = delete;

		// #todo 한번에 여러개의 input이 들어올 때 어떻게 처리하지?
		void BindAction(String nameOfAction, EInput input);
		void BindAxis(String nameOfAxis, EInput input, float scale);

		// #todo UnbindAction/Axis

		WeakHandle<Action> QueryAction(String nameOfAction) const;
		WeakHandle<Axis>	 QueryAxis(String nameOfAxis) const;
		float					 QueryScaleOfAxis(String nameOfAxis, EInput input) const;

		void SetScaleOfAxis(String nameOfAxis, EInput input, float scale);

		void HandleEvent(UINT message, WPARAM wParam, LPARAM lParam);
		void PostUpdate();

	private:
		void HandleKeyDown(WPARAM wParam, LPARAM lParam);
		void HandleKeyUp(WPARAM wParam, LPARAM lParam);
		void HandleMouseMove(WPARAM wParam, LPARAM lParam);

		void HandlePressAction(EInput input);
		void HandleReleaseAction(EInput input);
		void HandleAxis(EInput input, float value);

	private:
		HandleManager& handleManager;

		InputNameMap	  inputActionNameMap{};
		InputNameScaleMap inputAxisNameScaleMap{};

		EventMap<Action> actionMap{};
		EventMap<Axis>	 axisMap{};

		float latestMouseX = std::numeric_limits<float>::infinity();
		float latestMouseY = std::numeric_limits<float>::infinity();

		robin_hood::unordered_set<EInput> preesedInputSet{};
	};
} // namespace fe
