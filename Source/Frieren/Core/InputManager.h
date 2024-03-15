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

	struct Action
	{
	public:
		bool IsAnyPressing() const { return State == fe::EInputState::Pressed || State == EInputState::OnPressing; }

	public:
		EInputState State = EInputState::None;

	private:
		float pad = 0.f;
	};

	struct Axis
	{
	public:
		float Value = 0.0f;

	private:
		float pad = 0.f;
	};

	class HandleManager;
	class InputManager
	{
		using NameSet = robin_hood::unordered_set<String>;
		using InputNameMap = robin_hood::unordered_map<EInput, NameSet>;
		using ScaleMap = robin_hood::unordered_map<String, float>;
		using InputNameScaleMap = robin_hood::unordered_map<EInput, ScaleMap>;
		template <typename T>
		using EventMap = robin_hood::unordered_map<String, Handle<T>>;

	public:
		InputManager(HandleManager& handleManager);
		InputManager(const InputManager&) = delete;
		InputManager(InputManager&&) noexcept = delete;

		InputManager& operator=(const InputManager&) = delete;
		InputManager& operator=(InputManager&&) noexcept = delete;

		void BindAction(String nameOfAction, EInput input);
		void BindAxis(String nameOfAxis, EInput input, const float scale = 1.f);

		// #sy_todo UnbindAction/Axis

		RefHandle<const Action> QueryAction(String nameOfAction) const;
		RefHandle<const Axis> QueryAxis(String nameOfAxis) const;
		float QueryScaleOfAxis(String nameOfAxis, EInput input) const;

		void SetScaleOfAxis(String nameOfAxis, EInput input, float scale);

		void HandleEvent(UINT message, WPARAM wParam, LPARAM lParam);
		void PostUpdate();

		void Clear();

	private:
		void HandleKeyDown(WPARAM wParam, LPARAM lParam);
		void HandleKeyUp(WPARAM wParam, LPARAM lParam);
		void HandleRawInputDevices(const WPARAM wParam, const LPARAM lParam);

		void HandlePressAction(EInput input);
		void HandleReleaseAction(EInput input);
		void HandleAxis(EInput input, float value);

	private:
		HandleManager& handleManager;

		InputNameMap inputActionNameMap{};
		InputNameScaleMap inputAxisNameScaleMap{};

		EventMap<Action> actionMap{};
		EventMap<Axis> axisMap{};

		robin_hood::unordered_set<EInput> preesedInputSet{};

		std::vector<uint8_t> rawInputBuffer;

	};
} // namespace fe
