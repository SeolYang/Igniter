#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/Result.h"
#include "Igniter/Core/String.h"

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

    struct DialogFilter final
    {
        String Name          = "All Files (*.*)"_fs; /* 이름에서의 필터 패턴은 선택 사항 */
        String FilterPattern = "*.*"_fs;
    };

    class OpenFileDialog final
    {
    public:
        static Result<String, EOpenFileDialogStatus> Show(
            const HWND parentWindowHandle, const String dialogTitle, const std::span<const DialogFilter> filters);
    };
} // namespace ig
