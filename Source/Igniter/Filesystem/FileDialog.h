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
        std::string_view Name = "All Files (*.*)"; /* 이름에서의 필터 패턴은 선택 사항 */
        std::string_view FilterPattern = "*.*";
    };

    class OpenFileDialog final
    {
    public:
        static Result<std::string, EOpenFileDialogStatus> Show(
            const HWND parentWindowHandle, const std::string_view dialogTitle, const std::span<const DialogFilter> filters);
    };
} // namespace ig
