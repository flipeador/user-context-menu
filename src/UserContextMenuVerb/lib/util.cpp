#include "../framework.h"

std::wstring MapStr(const std::string& str)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> c;
    return c.from_bytes(str);
}

std::string MapStr(const std::wstring& wstr)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> c;
    return c.to_bytes(wstr);
}

BOOL IsKeyDown(INT key)
{
    return GetAsyncKeyState(key) < 0;
}

BOOL IsDarkThemeEnabled()
{
    DWORD data, size = sizeof(data);
    SHGetValueA(HKEY_CURRENT_USER,
        "Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        "AppsUseLightTheme", nullptr, &data, &size);
    return data == 0 ? TRUE : FALSE;
}

String GetFullPathName(StrView str)
{
    String path;
    path.resize_and_overwrite(LONG_MAXPATH,
        [&str](wchar_t* ptr, size_t count) -> size_t {
            return GetFullPathNameW(str.data(), (DWORD)count, ptr, nullptr);
        }
    );
    return path;
}

String GetPathFromIDList(ITEMIDLIST* pIDL)
{
    String str;
    if (pIDL != nullptr)
        str.resize_and_overwrite(LONG_MAXPATH,
            [&pIDL](wchar_t* ptr, size_t count) -> size_t {
                auto r = SHGetPathFromIDListEx(pIDL, ptr, (DWORD)count, GPFIDL_DEFAULT);
                return r ? std::wcslen(ptr) : 0;
            }
        );
    return str;
}

String GetKnownFolderPath(const GUID& guid)
{
    wchar_t* pPath = nullptr;
    auto hr = SHGetKnownFolderPath(guid, KF_FLAG_DEFAULT, nullptr, &pPath);
    String str = SUCCEEDED(hr) ? pPath : L"";
    CoTaskMemFree(pPath);
    return str;
}

String GetModuleFileName(HMODULE hModule)
{
    String str;
    str.resize_and_overwrite(LONG_MAXPATH,
        [&hModule](wchar_t* ptr, size_t count) -> size_t {
            return GetModuleFileNameW(hModule, ptr, (DWORD)count);
        }
    );
    return str;
}

String GetEnvironmentVariable(StrView name)
{
    String value;
    value.resize_and_overwrite(
        GetEnvironmentVariableW(name.data(), nullptr, 0),
        [&name](wchar_t* ptr, size_t count) -> size_t {
            return GetEnvironmentVariableW(name.data(), ptr, (DWORD)count);
        }
    );
    return value;
}

BOOL SetEnvironmentVariable(StrView name, StrView value)
{
    return SetEnvironmentVariableW(name.data(), value.data());
}

String ExpandEnvironmentStrings(StrView strv)
{
    String str;
    str.resize_and_overwrite(
        ExpandEnvironmentStringsW(strv.data(), nullptr, 0),
        [&strv](wchar_t* ptr, size_t count) -> size_t {
            count = ExpandEnvironmentStringsW(strv.data(), ptr, (DWORD)count);
            return count == 0 ? 0 : count - 1; // minus terminating null char
        }
    );
    return str;
}

std::generator<String> ParseItems(StrView str, wchar_t sep)
{
    String item, temp;
    bool quote = false;
    for (const auto& chr : str)
    {
        if (chr == L'"')
            quote = !quote;
        else if (quote)
            item += chr;
        else if (chr != sep)
        {
            if (chr == L' ')
            {
                if (!item.empty())
                    temp += chr;
            }
            else
            {
                item += temp + chr;
                temp.clear();
            }
        }
        else if (!item.empty())
        {
            co_yield item;
            item.clear();
        }
    }
    if (!item.empty())
        co_yield item;
}

String FindPath(Path name, DWORD mask, DWORD attr)
{
    if (name.empty() || name.is_absolute())
        return name.lexically_normal();
    const auto path = GetEnvironmentVariable(L"PATH");
    for (const auto& item : ParseItems(path, L';'))
    {
        if (const auto path = item / name; path.is_absolute())
        {
            const auto attr2 = GetFileAttributesW(path.wstring().data());
            if (attr2 != INVALID_FILE_ATTRIBUTES && BITMSK(attr2, mask, attr))
                return path.lexically_normal();
        }
    }
    return { };
}

DWORD ShellExecute(HWND hWnd, StrView verb, StrView file, StrView args, StrView wdir, INT scmd)
{
    if (verb == L"MsgBox") // for debugging purposes
    {
        const auto text = std::format(L"File: {}\nArgs: {}\nWDir: {}", file, args, wdir);
        MessageBoxW(hWnd, text.data(), GetModuleFileName(g_hModule).data(), MB_OK);
        return NO_ERROR;
    }
  
    return (DWORD)(INT_PTR)ShellExecuteW(hWnd, STR_NULL_IF_EMPTY(verb),
        file.data(), STR_NULL_IF_EMPTY(args), STR_NULL_IF_EMPTY(wdir), scmd);
}
