#pragma once
#include <Igniter.h>
#include <Core/String.h>
#include <Core/Result.h>

namespace ig
{
    enum class EOpenFileDialogStatus
    {
        Success,
        CreateDialog,
        CreateEventHandler,
        HookUpEventHandler,
        GetDialogOptions,
        SetDialogOptions,
        SetFileTypes,
        SetFileTypeIndex,
        SetDefaultExtension,
        ShowDialog,
        GetResult,
        GetDisplayName
    };

    struct DialogFilter
    {
        std::wstring_view Name = L"All Files (*.*)"; /* 이름에서의 필터 패턴은 선택 사항 */
        std::wstring_view FilterPattern = L"*.*";
    };

    struct WindowDescription
    {
        const uint32_t Width;
        const uint32_t Height;
        const String Title;
    };

    // @dependency	ig::Engine, ig::Logger, ig::InputManager
    class Window final
    {
    public:
        Window(const WindowDescription& description);
        ~Window();
        Window(const Window&) = delete;
        Window(Window&&) noexcept = delete;

        Window& operator=(const Window&) = delete;
        Window& operator=(Window&&) noexcept = delete;

        const WindowDescription& GetDescription() const { return windowDesc; }

        HWND GetNative() const { return windowHandle; }

        [[nodiscard]] Viewport GetViewport() const { return viewport; }

        void ClipCursor(const std::optional<RECT> clipRect = std::nullopt);
        void UnclipCursor();

        void SetCursorVisibility(const bool bVisible);

        Result<fs::path, EOpenFileDialogStatus> OpenFileDialog(const std::span<const DialogFilter> filters);

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