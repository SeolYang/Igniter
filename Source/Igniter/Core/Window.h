#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/String.h"

namespace ig
{
    struct WindowDescription
    {
        U32 Width;
        U32 Height;
        std::string_view Title;
    };

    // @dependency	ig::Engine, ig::Logger, ig::InputManager
    class Window final
    {
    public:
        explicit Window(const WindowDescription& description);
        ~Window();
        Window(const Window&) = delete;
        Window(Window&&) noexcept = delete;

        Window& operator=(const Window&) = delete;
        Window& operator=(Window&&) noexcept = delete;

        const WindowDescription& GetDescription() const { return windowDesc; }

        HWND GetNative() const { return windowHandle; }

        [[nodiscard]] Viewport GetViewport() const noexcept { return viewport; }

        void ClipCursor(const std::optional<RECT> clipRect = std::nullopt);
        void UnclipCursor();

        void SetCursorVisibility(const bool bVisible);

        void PumpMessage();

    private:
        static LRESULT CALLBACK WindowProc(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);

    private:
        WindowDescription windowDesc;

        std::wstring windowTitle;
        WNDCLASSEX windowClass;
        HWND windowHandle = nullptr;

        Viewport viewport;

        bool bIsCursorVisible = true;
    };
} // namespace ig
