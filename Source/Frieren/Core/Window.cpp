#include <Core/Window.h>
#include <Core/Assert.h>
#include <Core/EmbededSettings.h>
#include <Core/InputManager.h>
#include <ImGui/Common.h>
#include <Engine.h>

namespace fe
{
	Window::Window(const WindowDescription& description)
		: windowDesc(description)
	{
		windowTitle = description.Title.AsWideString();
		check(description.Width > 0 && description.Height > 0);
		windowClass = { sizeof(WNDCLASSEX),
						CS_OWNDC,
						WindowProc,
						0L,
						0L,
						GetModuleHandle(NULL),
						NULL,
						LoadCursor(NULL, IDC_ARROW),
						NULL,
						NULL,
						windowTitle.c_str(),
						NULL };

		verify(SUCCEEDED(RegisterClassEx(&windowClass)));
		const auto screenWidth = GetSystemMetrics(SM_CXSCREEN);
		const auto screenHeight = GetSystemMetrics(SM_CYSCREEN);

		constexpr auto windowStyle = WS_POPUP;
		constexpr auto exWindowStyle = 0;

		rect = RECT{ 0, static_cast<LONG>(description.Height), static_cast<LONG>(description.Width), 0 };
		verify(AdjustWindowRectEx(&rect, windowStyle, false, exWindowStyle) != FALSE);

		windowHandle = CreateWindowEx(exWindowStyle, windowClass.lpszClassName, windowClass.lpszClassName, windowStyle,
									  (screenWidth - description.Width) / 2, (screenHeight - description.Height) / 2,
									  rect.right - rect.left, rect.top - rect.bottom, NULL, NULL,
									  windowClass.hInstance, NULL);

		viewport.width = static_cast<float>(rect.right - rect.left);
		viewport.height = static_cast<float>(rect.top - rect.bottom);

		ShowWindow(windowHandle, SW_SHOWDEFAULT);
		UpdateWindow(windowHandle);
	}

	Window::~Window()
	{
		UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
	}

	LRESULT CALLBACK Window::WindowProc(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
	{
		if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		{
			return true;
		}

		switch (uMsg)
		{
			case WM_DESTROY:
				PostQuitMessage(0);
				return 0;

			default:
				break;
		}

		Engine::GetInputManager().HandleEvent(uMsg, wParam, lParam);
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}


} // namespace fe