#include "../framework.hpp"

/***************************************************
 * STRING
***************************************************/

//static size_t ToMultiByte(const wchar_t* pwc, size_t wcs, char* pc, size_t cs)
//{
//    if (pc == nullptr) cs = 0;
//    if (wcs > INT_MAX || cs > INT_MAX) throw std::overflow_error("");
//    return WideCharToMultiByte(CP_UTF8, 0, pwc, (INT)wcs, pc, (INT)cs, nullptr, nullptr);
//}

static size_t ToWideChar(const char* pc, size_t cs, wchar_t* pwc, size_t wcs)
{
    if (pwc == nullptr) wcs = 0;
    if (cs > INT_MAX || wcs > INT_MAX) throw std::overflow_error("");
    return MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, pc, (INT)cs, pwc, (INT)wcs);
}

String MapStr(std::string_view str)
{
    String wstr;
    if (!str.empty())
        wstr.resize_and_overwrite(
            ToWideChar(str.data(), str.size(), nullptr, 0),
            [&str](wchar_t* ptr, size_t count) -> size_t {
                return ToWideChar(str.data(), str.size(), ptr, count);
            }
        );
    return wstr;
}

/***************************************************
 * MISC
***************************************************/

BOOL IsKeyDown(IList<INT> keys)
{
    return std::all_of(keys.begin(), keys.end(),
        [](INT key) { return GetAsyncKeyState(key) < 0; });
}

BOOL IsDarkThemeEnabled()
{
    DWORD data, size = sizeof(data);
    SHGetValueA(HKEY_CURRENT_USER,
        "Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        "AppsUseLightTheme", nullptr, &data, &size);
    return data == 0 ? TRUE : FALSE;
}

String GetShellItemIDListPath(LPCITEMIDLIST pIDL)
{
    String str;
    if (pIDL != nullptr)
        str.resize_and_overwrite(PATH_MAX,
            [&pIDL](wchar_t* ptr, size_t count) -> size_t {
                auto r = SHGetPathFromIDListEx(pIDL, ptr, (DWORD)count, GPFIDL_DEFAULT);
                return r ? std::wcslen(ptr) : 0;
            }
        );
    return str;
}

String GetKnownFolderPath(RIID iid)
{
    wchar_t* pPath = nullptr;
    auto hr = SHGetKnownFolderPath(iid, KF_FLAG_DEFAULT, nullptr, &pPath);
    String str = SUCCEEDED(hr) ? pPath : L"";
    CoTaskMemFree(pPath);
    return str;
}

String GetModulePath(HMODULE hModule)
{
    String str;
    str.resize_and_overwrite(PATH_MAX,
        [&](wchar_t* ptr, size_t count) -> size_t {
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
        [&](wchar_t* ptr, size_t count) -> size_t {
            return GetEnvironmentVariableW(name.data(), ptr, (DWORD)count);
        }
    );
    return value;
}

BOOL SetEnvironmentVariable(StrView name, const Optional<StrView>& value)
{
    return SetEnvironmentVariableW(name.data(), STR_OPT_DATA(value));
}

String ExpandEnvironmentStrings(StrView strv)
{
    String str;
    str.resize_and_overwrite(
        ExpandEnvironmentStringsW(strv.data(), nullptr, 0),
        [&](wchar_t* ptr, size_t count) -> size_t {
            count = ExpandEnvironmentStringsW(strv.data(), ptr, (DWORD)count);
            return count == 0 ? 0 : count - 1; // minus terminating null char
        }
    );
    return str;
}

Optional<DWORD> GetFileAttributes(StrView path)
{
    auto attr = GetFileAttributesW(path.data());
    if (attr == INVALID_FILE_ATTRIBUTES)
        return std::nullopt;
    return attr;
}

Generator<String> ParseItems(StrView str, WCHAR sep)
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
            const auto attr2 = GetFileAttributes(path.wstring());
            if (attr2 && BITMSK(*attr2, mask, attr))
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
        MessageBoxW(hWnd, text.data(), PACKAGE_NAME, MB_OK);
        return NO_ERROR;
    }

    return (DWORD)(INT_PTR)ShellExecuteW(hWnd, STR_NULL_IF_EMPTY(verb),
        file.data(), STR_NULL_IF_EMPTY(args), STR_NULL_IF_EMPTY(wdir), scmd);
}

Optional<String> DragQueryFile(HDROP hDrop, UINT index)
{
    String path;
    path.resize_and_overwrite(
        DragQueryFileW(hDrop, index, nullptr, 0),
        [&](wchar_t* ptr, size_t count) -> size_t {
            return DragQueryFileW(hDrop, index, ptr, UINT(count + 1));
        }
    );
    if (path.empty()) return std::nullopt; return path;
}

/***************************************************
 * DIALOGS
***************************************************/

Optional<Pair<ComStr, INT>> PickIcon(HWND hWnd, StrView path, INT index)
{
    auto str = ComAllocStr(PATH_MAX, path);
    if (PickIconDlg(hWnd, str.get(), PATH_MAX, &index))
        return Pair(std::move(str), index);
    return std::nullopt;
}
