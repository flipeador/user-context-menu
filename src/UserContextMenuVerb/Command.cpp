#include "framework.hpp"

#define SHOW_FILE_DIALOG(function, interface, index, options)                                         \
    function([&](interface& dialog) {                                                                 \
        IShellItem* pFolder;                                                                          \
        HRESULT hr = SHCreateItemFromParsingName(path.data(), nullptr, IID_PPV_ARGS(&pFolder));       \
        if (SUCCEEDED(hr))                                                                            \
        {                                                                                             \
            dialog.SetFolder(pFolder);                                                                \
            dialog.SetFileName(match.str(index).data());                                              \
            dialog.SetOptions(options | FOS_NOCHANGEDIR | FOS_FORCEFILESYSTEM | FOS_DONTADDTORECENT); \
            hr = dialog.Show(hWnd);                                                                   \
            pFolder->Release();                                                                       \
        }                                                                                             \
        return hr;                                                                                    \
    })

static auto ExpandCommandVars(StrView path, StrView str)
{
    SetCommandEnvVarsPath(path);
    return ExpandEnvironmentStrings(STR_NOT_EMPTY_OR(str, L"\"%:PATH%\""));
}

static auto ExpandCommandDialogVars(String& str, HWND hWnd, StrView path)
{
    return
        // FileOpenDialog
        // %:FILE[S][:<filename>:]%
        // %:FOLDER[S][:<filename>:]%
        StrReplace(str, std::wregex(L"%:(FILE|FOLDER)(S)?(:(.*?):)?%"),
            [&](const std::wsmatch& match) -> Optional<String> {
                auto items = SHOW_FILE_DIALOG(
                    FileOpenDialog, IFileOpenDialog, 4,
                    FOS_NOVALIDATE | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST |
                    (match.str(1) == L"FILE" ? 0 : FOS_PICKFOLDERS)        |
                    (match.str(2).empty()    ? 0 : FOS_ALLOWMULTISELECT)
                );
                if (items.empty()) return std::nullopt;
                return items[0];
            }
        ) &&
        // FileSaveDialog
        // %:FILESAVE[:<filename>:]%
        StrReplace(str, std::wregex(L"%:FILESAVE(:(.*?):)?%"),
            [&](const std::wsmatch& match) -> Optional<String> {
                auto item = SHOW_FILE_DIALOG(
                    FileSaveDialog, IFileSaveDialog, 2,
                    FOS_NOTESTFILECREATE | FOS_OVERWRITEPROMPT
                );
                if (item.empty()) return std::nullopt;
                return item;
            }
        );
}

static auto ExecuteCommand(HWND hWnd, StrView verb, StrView file, StrView cmdl, StrView args, StrView wdir, INT scmd)
{
    SetEnvironmentVariable(L":PATH", args);

    auto cmdl2 = ExpandEnvironmentStrings(STR_NOT_EMPTY_OR(cmdl, L"%:PATH%"));
    auto wdir2 = ExpandEnvironmentStrings(STR_NOT_EMPTY_OR(wdir, L"%:BACKGROUND%"));

    if (!ExpandCommandDialogVars(cmdl2, hWnd, wdir2)) return;

    // Displays security prompts and user interface (UI) error dialogs.
    const auto r = ShellExecute(hWnd, verb, file, cmdl2, wdir2, scmd, SEE_MASK_DEFAULT);
    const DWORD error = r == NULL ? GetLastError() : NO_ERROR;

    SEND_DEBUG_MESSAGE(
        L"ShellExecute:\n\tHWND={}\n\tVERB={}\n\tFILE={}\n\tCMDL={}\n\tWDIR={}\n\tSCMD={}\n\tERROR={}",
        (PVOID)hWnd, verb, file, cmdl2, wdir2, scmd, error
    );
}

String FindFilePath(StrView str, StrView defs)
{
    const auto expanded = ExpandEnvironmentStrings(str);
    auto path = FindPath(expanded, FILE_ATTRIBUTE_DIRECTORY);
    return path.empty() ? String(defs) : path;
}

VOID SetCommandEnvVarsPath(const Path& path)
{
    SetEnvironmentVariable(L":PARENT", path.parent_path().wstring());
    SetEnvironmentVariable(L":PATH", path.wstring());
    SetEnvironmentVariable(L":NAME", path.filename().wstring());
    SetEnvironmentVariable(L":STEM", path.stem().wstring());
    SetEnvironmentVariable(L":EXT", path.extension().wstring());
}

Pair<String,INT> GetCommandIcon(const Json& json)
{
    size_t n = g_pInitObj->isDarkTheme; // 0 = light | 1 = dark
    const auto& icons = json["icons"]; // [ [light,index] , [dark,index] ]
    return { FindFilePath(JSON_GET_WSTR(icons[n][0])), (INT)icons[n][1] };
}

HRESULT Command::Invoke(HWND hWnd, const Json& json)
{
    auto scmd = json["scmd"].get<SHORT>();
    auto verb = JSON_GET_WSTR(json["verb"]);
    auto file = JSON_GET_WSTR(json["file"]);
    auto cmdl = JSON_GET_WSTR(json["args"]);
    auto wdir = JSON_GET_WSTR(json["wdir"]);
    auto mode = JSON_GET_STR(json["multi"]["mode"]);

    if (IsKeyDown({ VK_CONTROL, VK_SHIFT }))
        verb = L"RunAs"; // force elevated

    SetEnvironmentVariable(L":ICON", GetCommandIcon(json).first);
    SetEnvironmentVariable(L":BACKGROUND", background);

    file = FindFilePath(file, file);

    if (mode == "Off")
    {
        const auto& path = items.empty() ? background : items[0].first;
        const auto args = ExpandCommandVars(path, cmdl);
        ExecuteCommand(hWnd, verb, file, L"", args, wdir, scmd);
    }
    else if (mode == "Each" || mode == "Join")
    {
        auto args = JSON_GET_WSTR(json["multi"]["args"]);

        if (items.empty())
        {
            args = ExpandCommandVars(background, args);
            ExecuteCommand(hWnd, verb, file, cmdl, args, wdir, scmd);
        }
        else if (mode == "Each")
        {
            for (const auto& [item, attr] : items)
            {
                args = ExpandCommandVars(item, args);
                ExecuteCommand(hWnd, verb, file, cmdl, args, wdir, scmd);
            }
        }
        else if (mode == "Join")
        {
            if (wdir.empty() && args.empty())
                args = L"\"%:NAME%\"";

            String args2;
            for (const auto& [item, attr] : items)
            {
                if (!args2.empty()) args2.push_back(L' ');
                args2.append(ExpandCommandVars(item, args));
            }

            ExecuteCommand(hWnd, verb, file, cmdl, args2, wdir, scmd);
        }
    }

    return S_OK;
}
