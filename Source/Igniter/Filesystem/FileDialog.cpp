#include "Igniter/Igniter.h"
#include "Igniter/Core/ComInitializer.h"
#include "Igniter/Filesystem/FileDialog.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "propsys.lib")
#pragma comment(lib, "shlwapi.lib")

namespace ig
{
    struct WideDialogFilter
    {
        std::wstring Name;
        std::wstring FilterPattern;
    };

    Result<String, EOpenFileDialogStatus> OpenFileDialog::Show(
        const HWND parentWindowHandle, const String dialogTitle, const std::span<const DialogFilter> filters)
    {
        CoInitializeUnique();
        Microsoft::WRL::ComPtr<IFileDialog> fileDialog;
        Microsoft::WRL::ComPtr<IFileDialogEvents> fileDialogEvents;
        Microsoft::WRL::ComPtr<IShellItem> shellItem;

        // #sy_note Save 다이얼로그엔 CLSID_FileSaveDialog
        HRESULT result = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&fileDialog));
        if (FAILED(result))
        {
            return MakeFail<String, EOpenFileDialogStatus::CreateDialog>();
        }

        Vector<WideDialogFilter> wideFilters(filters.size());
        std::transform(filters.begin(), filters.end(), wideFilters.begin(),
            [](const DialogFilter& filter)
            {
                return WideDialogFilter{.Name = filter.Name.ToWideString(), .FilterPattern = filter.FilterPattern.ToWideString()};
            });

        Vector<COMDLG_FILTERSPEC> filterSpecs(filters.size());
        std::transform(wideFilters.begin(), wideFilters.end(), filterSpecs.begin(),
            [](const WideDialogFilter& filter)
            {
                return COMDLG_FILTERSPEC{.pszName = filter.Name.c_str(), .pszSpec = filter.FilterPattern.c_str()};
            });

        result = fileDialog->SetFileTypes(static_cast<U32>(filterSpecs.size()), filterSpecs.data());
        if (FAILED(result))
        {
            return MakeFail<String, EOpenFileDialogStatus::SetFileTypes>();
        }

        fileDialog->SetTitle(dialogTitle.ToWideString().c_str());

        result = fileDialog->Show(parentWindowHandle);
        if (FAILED(result))
        {
            return MakeFail<String, EOpenFileDialogStatus::ShowDialog>();
        }

        result = fileDialog->GetResult(&shellItem);
        if (FAILED(result))
        {
            return MakeFail<String, EOpenFileDialogStatus::GetResult>();
        }

        PWSTR filePath{nullptr};
        result = shellItem->GetDisplayName(SIGDN_FILESYSPATH, &filePath);
        if (FAILED(result))
        {
            return MakeFail<String, EOpenFileDialogStatus::GetDisplayName>();
        }

        String resultPath{Narrower(filePath)};
        CoTaskMemFree(filePath);
        return MakeSuccess<String, EOpenFileDialogStatus>(resultPath);
    }
} // namespace ig
