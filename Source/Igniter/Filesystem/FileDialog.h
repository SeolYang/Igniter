#pragma once
#include <Igniter.h>
#include <Core/Result.h>
#include <Core/String.h>

namespace ig
{
    enum class EOpenFileDialogStatus
    {
        Success,
        CreateDialog,
        SetFileTypes,
        ShowDialog,
        GetResult,
        GetDisplayName
    };

    struct DialogFilter
    {
        String Name = "All Files (*.*)"_fs; /* 이름에서의 필터 패턴은 선택 사항 */
        String FilterPattern = "*.*"_fs;
    };

    class OpenFileDialog
    {
    public:
        static Result<String, EOpenFileDialogStatus> Show(const HWND parentWindowHandle, const String dialogTitle, const std::span<const DialogFilter> filters);
    };
} // namespace ig