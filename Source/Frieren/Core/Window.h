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

	class Window final
	{
	public:
		Window(WindowDescription description);
		~Window();
		Window(const Window&) = delete;
		Window(Window&&) = delete;

		Window& operator=(const Window&) = delete;
		Window& operator=(Window&&) = delete;

	private:
		static LRESULT CALLBACK WindowProc(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);

	private:
		WNDCLASSEX windowClass;
		HWND	   windowHandle = NULL;
	};
} // namespace fe