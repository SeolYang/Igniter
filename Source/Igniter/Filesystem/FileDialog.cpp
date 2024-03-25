#include <Igniter.h>
#include <Core/ComInitializer.h>
#include <Filesystem/FileDialog.h>

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

    Result<fs::path, EOpenFileDialogStatus> OpenFileDialog::Show(const HWND parentWindowHandle, const String dialogTitle, const std::span<const DialogFilter> filters)
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

        std::vector<WideDialogFilter> wideFilters(filters.size());
        std::transform(filters.begin(), filters.end(), wideFilters.begin(),
                       [](const DialogFilter& filter)
                       {
                           return WideDialogFilter{ .Name = filter.Name.AsWideString(), .FilterPattern = filter.FilterPattern.AsWideString() };
                       });

        std::vector<COMDLG_FILTERSPEC> filterSpecs(filters.size());
        std::transform(wideFilters.begin(), wideFilters.end(), filterSpecs.begin(),
                       [](const WideDialogFilter& filter)
                       {
                           return COMDLG_FILTERSPEC{ .pszName = filter.Name.c_str(), .pszSpec = filter.FilterPattern.c_str() };
                       });

        result = fileDialog->SetFileTypes(static_cast<uint32_t>(filterSpecs.size()), filterSpecs.data());
        if (FAILED(result))
        {
            return MakeFail<fs::path, EOpenFileDialogStatus::SetFileTypes>();
        }

        fileDialog->SetTitle(dialogTitle.AsWideString().c_str());

        result = fileDialog->Show(parentWindowHandle);
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

} // namespace ig