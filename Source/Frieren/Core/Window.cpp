#include <Core/Window.h>
#include <Core/Assert.h>
#include <Core/EmbededSettings.h>

namespace fe
{
	Window::Window(const WindowDescription description)
	{
		const auto title = description.Title.AsStringView();
		constexpr size_t TitleBufferSize = 256;
		wchar_t	   wcharTitle[TitleBufferSize];
		MultiByteToWideChar(CP_UTF8, 0, title.data(), -1, wcharTitle, TitleBufferSize);

		FE_ASSERT(description.Width > 0 && description.Height > 0, "Invalid Window Resolution.");
		windowClass = {
			sizeof(WNDCLASSEX),
			CS_OWNDC,
			WindowProc, 0L, 0L,
			GetModuleHandle(NULL), NULL, LoadCursor(NULL, IDC_ARROW), NULL, NULL,
			wcharTitle, NULL
		};

		if (RegisterClassEx(&windowClass))
		{
			const auto screenWidth = GetSystemMetrics(SM_CXSCREEN);
			const auto screenHeight = GetSystemMetrics(SM_CYSCREEN);

			constexpr auto windowStyle = WS_OVERLAPPEDWINDOW;
			constexpr auto exWindowStyle = 0;

			RECT winRect{
				0,
				static_cast<LONG>(description.Height),
				static_cast<LONG>(description.Width),
				0
			};

			FE_ASSERT(AdjustWindowRectEx(&winRect, windowStyle, false, exWindowStyle) != 0, "Failed to adjust window rect");

			windowHandle = CreateWindowEx(
				exWindowStyle,
				windowClass.lpszClassName,
				wcharTitle,
				windowStyle,
				(screenWidth - description.Width) / 2, (screenHeight - description.Height) / 2,
				winRect.right - winRect.left, winRect.top - winRect.bottom,
				NULL, NULL, windowClass.hInstance, NULL);

			ShowWindow(windowHandle, SW_SHOWDEFAULT);
			UpdateWindow(windowHandle);
		}
		else
		{
			FE_ASSERT(false, "Failed to register window class.");
		}
	}

	Window::~Window()
	{
		UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
	}

	LRESULT CALLBACK Window::WindowProc(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
	{
		switch (uMsg)
		{
			case WM_DESTROY:
				PostQuitMessage(0);
				return 0;

			default:
				break;
		}

		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
} // namespace fe