#include "framework.hpp"

static auto ExpandCommandVars(Path path, StrView str)
{
    SetEnvironmentVariable(L":PARENT", path.parent_path().wstring());
    SetEnvironmentVariable(L":PATH", path.wstring());
    SetEnvironmentVariable(L":NAME", path.filename().wstring());
    SetEnvironmentVariable(L":STEM", path.stem().wstring());
    SetEnvironmentVariable(L":EXT", path.extension().wstring());
    return ExpandEnvironmentStrings(STR_NOT_EMPTY_OR(str, L"\"%:PATH%\""));
}

static auto ExecCommand(HWND hWnd, StrView verb, StrView file, StrView cmdl, StrView args, StrView wdir, INT scmd)
{
    SetEnvironmentVariable(L":PATH", args);
    auto cmdl2 = ExpandEnvironmentStrings(STR_NOT_EMPTY_OR(cmdl, L"%:PATH%"));
    auto wdir2 = ExpandEnvironmentStrings(STR_NOT_EMPTY_OR(wdir, L"%:BACKGROUND%"));
    ShellExecute(hWnd, verb, file, cmdl2, wdir2, scmd);
}

String FindFilePath(StrView str, StrView defs)
{
    const auto expanded = ExpandEnvironmentStrings(str);
    auto path = FindPath(expanded, FILE_ATTRIBUTE_DIRECTORY);
    return path.empty() ? String(defs) : path;
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

    SetEnvironmentVariable(L":BACKGROUND", background);

    file = FindFilePath(file, file);

    if (mode == "Off")
    {
        const auto& path = items.empty() ? background : items[0].first;
        ExecCommand(hWnd, verb, file, L"", ExpandCommandVars(path, cmdl), wdir, scmd);
    }
    else if (mode == "Each" || mode == "Join")
    {
        auto args = JSON_GET_WSTR(json["multi"]["args"]);

        if (items.empty())
        {
            const auto args2 = ExpandCommandVars(background, args);
            ExecCommand(hWnd, verb, file, cmdl, args2, wdir, scmd);
        }
        else if (mode == "Each")
        {
            for (const auto& [item, attr] : items)
            {
                const auto args2 = ExpandCommandVars(item, args);
                ExecCommand(hWnd, verb, file, cmdl, args2, wdir, scmd);
            }
        }
        else if (mode == "Join")
        {
            if (wdir.empty() && args.empty())
                args = L"\"%:NAME%\"";

            String args2;
            for (const auto& [item, attr] : items)
                args2 += ExpandCommandVars(item, args) + L" ";

            ExecCommand(hWnd, verb, file, cmdl, args2, wdir, scmd);
        }
    }

    return S_OK;
}
