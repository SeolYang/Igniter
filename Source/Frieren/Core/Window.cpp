#include <Core/Window.h>
#include <Core/Assert.h>
#include <Core/EmbededSettings.h>
#include <Core/InputManager.h>
#include <ImGui/Common.h>
#include <Engine.h>

namespace fe
{
	FE_DEFINE_LOG_CATEGORY(WindowInfo, ELogVerbosity::Info)
	FE_DEFINE_LOG_CATEGORY(WindowDebug, ELogVerbosity::Debug)
	FE_DEFINE_LOG_CATEGORY(WindowError, ELogVerbosity::Error)

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

		RECT rect{ 0, static_cast<LONG>(description.Height), static_cast<LONG>(description.Width), 0 };
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

	void Window::ClipCursor(const std::optional<RECT> clipRect)
	{
		RECT clientRect{};
		if (clipRect)
		{
			clientRect = *clipRect;
		}
		else
		{
			GetClientRect(windowHandle, &clientRect);
		}

		POINT leftTop{ .x = clientRect.left, .y = clientRect.top };
		POINT rightBottom{ .x = clientRect.right, .y = clientRect.bottom };
		ClientToScreen(windowHandle, &leftTop);
		ClientToScreen(windowHandle, &rightBottom);

		clientRect.left = leftTop.x;
		clientRect.top = leftTop.y;
		clientRect.right = rightBottom.x;
		clientRect.bottom = rightBottom.y;
		if (::ClipCursor(&clientRect) == FALSE)
		{
			FE_LOG(WindowError, "Failed to clip cursor to main window.");
		}
		else
		{
			FE_LOG(WindowInfo, "Cursor clipped to rect(LEFT: {}, RIGHT: {}, TOP: {}, BOTTOM: {})", clientRect.left, clientRect.right, clientRect.top, clientRect.bottom);
		}
	}

	void Window::UnclipCursor()
	{
		::ClipCursor(nullptr);
	}

	void Window::SetCursorVisibility(const bool bVisible)
	{
		if (bVisible && !bIsCursorVisible)
		{
			bIsCursorVisible = true;
			::ShowCursor(TRUE);
			FE_LOG(WindowInfo, "Cursor visible.");
		}
		else if (!bVisible && bIsCursorVisible)
		{
			bIsCursorVisible = false;
			::ShowCursor(FALSE);
			FE_LOG(WindowInfo, "Cursor invisible");
		}
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