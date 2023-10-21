#pragma once
#include <cstdint>
#include <Core/Win32API.h>
#include <Core/String.h>
#include <Core/Log.h>

namespace fe
{
	struct WindowDescription
	{
		const uint32_t Width;
		const uint32_t Height;
		const String   Title;
	};

	// @dependency	fe::Engine, fe::Logger, fe::InputManager
	class Window final
	{
	public:
		Window(WindowDescription description);
		~Window();
		Window(const Window&) = delete;
		Window(Window&&) noexcept = delete;

		Window& operator=(const Window&) = delete;
		Window& operator=(Window&&) noexcept = delete;

		WindowDescription GetDescription() const { return windowDesc; }

	private:
		static LRESULT CALLBACK WindowProc(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);

	private:
		WindowDescription windowDesc;

		WNDCLASSEX windowClass;
		HWND	   windowHandle = NULL;
	};
} // namespace fe