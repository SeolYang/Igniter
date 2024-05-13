#include "Igniter/Igniter.h"
#include "Igniter/Core/EmbededSettings.h"
#include "Igniter/Core/Log.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Input/InputManager.h"
#include "Igniter/ImGui/ImGuiContext.h"
#include "Igniter/Core/Window.h"

IG_DEFINE_LOG_CATEGORY(Window);

namespace ig
{
    Window::Window(const WindowDescription& description) : windowDesc(description)
    {
        windowTitle = description.Title.ToWideString();
        IG_CHECK(description.Width > 0 && description.Height > 0);
        windowClass = {sizeof(WNDCLASSEX), CS_OWNDC, WindowProc, 0L, 0L, GetModuleHandle(NULL), NULL, LoadCursor(NULL, IDC_ARROW), NULL, NULL,
            windowTitle.c_str(), NULL};

        IG_VERIFY(SUCCEEDED(RegisterClassEx(&windowClass)));
        const auto screenWidth = GetSystemMetrics(SM_CXSCREEN);
        const auto screenHeight = GetSystemMetrics(SM_CYSCREEN);

        constexpr auto windowStyle = WS_POPUP;
        constexpr auto exWindowStyle = 0;

        RECT rect{0, static_cast<LONG>(description.Height), static_cast<LONG>(description.Width), 0};
        IG_VERIFY(AdjustWindowRectEx(&rect, windowStyle, false, exWindowStyle) != FALSE);

        windowHandle =
            CreateWindowEx(exWindowStyle, windowClass.lpszClassName, windowClass.lpszClassName, windowStyle, (screenWidth - description.Width) / 2,
                (screenHeight - description.Height) / 2, rect.right - rect.left, rect.top - rect.bottom, NULL, NULL, windowClass.hInstance, NULL);

        viewport.width = static_cast<float>(rect.right - rect.left);
        viewport.height = static_cast<float>(rect.top - rect.bottom);

        ShowWindow(windowHandle, SW_SHOWDEFAULT);
        UpdateWindow(windowHandle);
    }

    Window::~Window()
    {
        UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
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

        POINT leftTop{.x = clientRect.left, .y = clientRect.top};
        POINT rightBottom{.x = clientRect.right, .y = clientRect.bottom};
        ClientToScreen(windowHandle, &leftTop);
        ClientToScreen(windowHandle, &rightBottom);

        clientRect.left = leftTop.x;
        clientRect.top = leftTop.y;
        clientRect.right = rightBottom.x;
        clientRect.bottom = rightBottom.y;
        if (::ClipCursor(&clientRect) == FALSE)
        {
            IG_LOG(Window, Error, "Failed to clip cursor to main window.");
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
        }
        else if (!bVisible && bIsCursorVisible)
        {
            bIsCursorVisible = false;
            ::ShowCursor(FALSE);
        }
    }

    void Window::PumpMessage()
    {
        ZoneScoped;
        MSG msg;
        ZeroMemory(&msg, sizeof(msg));
        if (PeekMessage(&msg, windowHandle, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        while (PeekMessage(&msg, windowHandle, 0, WM_MOUSEMOVE - 1, PM_REMOVE) || PeekMessage(&msg, windowHandle, WM_MOUSEMOVE + 1, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                Igniter::Stop();
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    LRESULT CALLBACK Window::WindowProc(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
    {
        ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

        switch (uMsg)
        {
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;

            default:
                break;
        }

        if (Igniter::IsInitialized())
        {
            Igniter::GetImGuiContext().HandleWindowProc(hWnd, uMsg, wParam, lParam);
            Igniter::GetInputManager().HandleEvent(uMsg, wParam, lParam);
        }

        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
}    // namespace ig
