#include <Igniter.h>
#include <Core/Window.h>
#include <Core/EmbededSettings.h>
#include <Core/InputManager.h>
#include <Core/Log.h>
#include <ImGUi/ImGuiCanvas.h>
#include <Core/ComInitializer.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "propsys.lib")
#pragma comment(lib, "shlwapi.lib")

namespace ig
{
    IG_DEFINE_LOG_CATEGORY(WindowInfo, ELogVerbosity::Info)
    IG_DEFINE_LOG_CATEGORY(WindowDebug, ELogVerbosity::Debug)
    IG_DEFINE_LOG_CATEGORY(WindowError, ELogVerbosity::Error)

    Window::Window(const WindowDescription& description)
        : windowDesc(description)
    {
        windowTitle = description.Title.AsWideString();
        IG_CHECK(description.Width > 0 && description.Height > 0);
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

        IG_VERIFY(SUCCEEDED(RegisterClassEx(&windowClass)));
        const auto screenWidth = GetSystemMetrics(SM_CXSCREEN);
        const auto screenHeight = GetSystemMetrics(SM_CYSCREEN);

        constexpr auto windowStyle = WS_POPUP;
        constexpr auto exWindowStyle = 0;

        RECT rect{ 0, static_cast<LONG>(description.Height), static_cast<LONG>(description.Width), 0 };
        IG_VERIFY(AdjustWindowRectEx(&rect, windowStyle, false, exWindowStyle) != FALSE);

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
            IG_LOG(WindowError, "Failed to clip cursor to main window.");
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

    Result<fs::path, EOpenFileDialogStatus> Window::OpenFileDialog(const std::span<const DialogFilter> filters)
    {
        CoInitializeUnique();
        Microsoft::WRL::ComPtr<IFileDialog> fileDialog;
        Microsoft::WRL::ComPtr<IFileDialogEvents> fileDialogEvents;
        Microsoft::WRL::ComPtr<IShellItem> shellItem;

        // #sy_log Save 다이얼로그엔 CLSID_FileSaveDialog
        HRESULT result = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&fileDialog));
        if (FAILED(result))
        {
            return MakeFail<fs::path, EOpenFileDialogStatus::CreateDialog>();
        }

        std::vector<COMDLG_FILTERSPEC> dialogFilters(filters.size());
        std::transform(filters.begin(), filters.end(), dialogFilters.begin(),
                       [](const DialogFilter& filter)
                       {
                           return COMDLG_FILTERSPEC{ .pszName = filter.Name.data(),
                                                     .pszSpec = filter.FilterPattern.data() };
                       });

        result = fileDialog->SetFileTypes(static_cast<uint32_t>(dialogFilters.size()), dialogFilters.data());
        fileDialog->SetTitle(L"A Single-Selection Dialog");
        if (FAILED(result))
        {
            return MakeFail<fs::path, EOpenFileDialogStatus::SetFileTypes>();
        }

        result = fileDialog->Show(windowHandle);
        if (FAILED(result))
        {
            return MakeFail<fs::path, EOpenFileDialogStatus::ShowDialog>();
        }

        result = fileDialog->GetResult(&shellItem);
        if (FAILED(result))
        {
            return MakeFail<fs::path, EOpenFileDialogStatus::GetResult>();
        }

        PWSTR filePath{ nullptr };
        result = shellItem->GetDisplayName(SIGDN_FILESYSPATH, &filePath);
        if (FAILED(result))
        {
            return MakeFail<fs::path, EOpenFileDialogStatus::GetDisplayName>();
        }

        fs::path resultPath{ filePath };
        CoTaskMemFree(filePath);
        return MakeSuccess<fs::path, EOpenFileDialogStatus>(resultPath);
    }

    LRESULT CALLBACK Window::WindowProc(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
    {
        const auto imguiCanvasRef = Igniter::TryGetImGuiCanvas();
        const bool bIgnoreImGuiInput = imguiCanvasRef ? imguiCanvasRef->get().IsIgnoreInputEnabled() : false;
        if (!bIgnoreImGuiInput && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        {
            return false;
        }

        switch (uMsg)
        {
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;

            default:
                break;
        }

        Igniter::GetInputManager().HandleEvent(uMsg, wParam, lParam);
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
} // namespace ig
