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

bool StrReplace(String& str, const std::wregex& pattern, const Function<Optional<String>(const std::wsmatch&)>& fn)
{
    String dest;
    auto r = StrReplace(dest, str, pattern, fn);
    str = std::move(dest);
    return r;
}

bool StrReplace(String& dest, const String& src, const std::wregex& pattern, const Function<Optional<String>(const std::wsmatch&)>& fn)
{
    std::wsmatch match;
    auto begin = src.cbegin();
    const auto last = src.cend();

    while (std::regex_search(begin, last, match, pattern))
    {
        dest.append(begin, match[0].first);
        const auto replacement = fn(match);
        if (!replacement) return false;
        dest.append(*replacement);
        begin = match[0].second;
    }

    dest.append(begin, last);

    return true;
}

/***************************************************
 * MISC
***************************************************/

BOOL IsKeyDown(IList<INT> keys)
{
    return std::all_of(keys.begin(), keys.end(),
        [](INT key) { return GetAsyncKeyState(key) < 0; });
}

INT GetSystemColorScheme()
{
    DWORD data, size = sizeof(data);
    SHGetValueW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"AppsUseLightTheme", nullptr, &data, &size);
    return data == 0 ? TRUE : FALSE; // 0 = Light | 1 = Dark
}

String GetModulePath(HMODULE hModule)
{
    String str;
    str.resize_and_overwrite(PATH_MAX - 1,
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
            item.push_back(chr);
        else if (chr != sep)
        {
            if (chr == L' ')
            {
                if (!item.empty())
                    temp.push_back(chr);
            }
            else
            {
                item.append(temp);
                item.push_back(chr);
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

String FindPath(Path name, DWORD attm, DWORD attv)
{
    if (name.empty() || name.is_absolute()) return name;
    const auto path = GetEnvironmentVariable(L"PATH");
    for (const auto& item : ParseItems(path, L';'))
    {
        if (const auto path = item / name; path.is_absolute())
        {
            const auto attr = GetFileAttributes(path.wstring());
            if (attr && BITMSK(*attr, attm, attv)) return path;
        }
    }
    return { };
}

String StringFromGUID(RGUID id)
{
    String str;
    str.resize_and_overwrite(GUID_MAX,
        [&](wchar_t* ptr, size_t count) -> size_t {
            return StringFromGUID2(id, ptr, (INT)count);
        }
    );
    return str;
}

String GetKnownFolderPath(RGUID id)
{
    PWSTR pPath = nullptr;
    const auto hr = SHGetKnownFolderPath(id, KF_FLAG_DEFAULT, NULL, &pPath);
    String str = SUCCEEDED(hr) ? pPath : L"";
    CoTaskMemFree(pPath);
    return str;
}

String GetItemIDListPath(LPCITEMIDLIST pIDL)
{
    String str;
    if (pIDL != nullptr)
        str.resize_and_overwrite(PATH_MAX - 1,
            [&pIDL](wchar_t* ptr, size_t count) -> size_t {
                auto r = SHGetPathFromIDListEx(pIDL, ptr, (DWORD)count, GPFIDL_DEFAULT);
                return r ? std::wcslen(ptr) : 0;
            }
        );
    return str;
}

HANDLE ShellExecute(HWND hWnd, StrView verb, StrView file, StrView args, StrView wdir, INT scmd, UINT flags)
{
    if (verb == L"MsgBox") // for debugging purposes
    {
        const auto text = std::format(L"File: {}\nArgs: {}\nWDir: {}", file, args, wdir);
        MessageBoxW(hWnd, text.data(), PACKAGE_NAME, MB_OK);
        return INVALID_HANDLE_VALUE;
    }

    SHELLEXECUTEINFOW sei {
        sizeof(SHELLEXECUTEINFOW),
        flags,
        hWnd,
        STR_NULL_IF_EMPTY(verb),
        STR_NULL_IF_EMPTY(file),
        STR_NULL_IF_EMPTY(args),
        STR_NULL_IF_EMPTY(wdir),
        scmd
    };

    if (!ShellExecuteExW(&sei)) return NULL; // error
    return sei.hProcess ? sei.hProcess : INVALID_HANDLE_VALUE;
}

//Optional<String> DragQueryFile(HDROP hDrop, UINT index)
//{
//    String path;
//    path.resize_and_overwrite(
//        DragQueryFileW(hDrop, index, nullptr, 0),
//        [&](wchar_t* ptr, size_t count) -> size_t {
//            return DragQueryFileW(hDrop, index, ptr, UINT(count + 1));
//        }
//    );
//    if (path.empty()) return std::nullopt; return path;
//}

/***************************************************
 * MESSAGES
***************************************************/

Optional<LRESULT> SendMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT timeout, UINT flags)
{
    DWORD_PTR lResult = 0;
    if (SendMessageTimeoutW(hWnd, msg, wParam, lParam, flags, timeout, &lResult))
        return static_cast<LRESULT>(lResult);
    return std::nullopt;
}

/***************************************************
 * DIALOGS
***************************************************/

String FileSaveDialog(const Function<HRESULT(IFileSaveDialog&)>& fn)
{
    String item;
    IFileSaveDialog* pFileSaveDialog;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileSaveDialog))))
    {
        if (SUCCEEDED(fn(*pFileSaveDialog)))
        {
            IShellItem* pItem;
            if (SUCCEEDED(pFileSaveDialog->GetResult(&pItem)))
            {
                PWSTR pName;
                if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pName)))
                {
                    item.assign(pName);
                    CoTaskMemFree(pName);
                }
                pItem->Release();
            }
        }
        pFileSaveDialog->Release();
    }
    return item;
}

Vector<String> FileOpenDialog(const Function<HRESULT(IFileOpenDialog&)>& fn)
{
    Vector<String> items;
    IFileOpenDialog* pFileOpenDialog;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileOpenDialog))))
    {
        if (SUCCEEDED(fn(*pFileOpenDialog)))
        {
            IShellItemArray* pItems;
            if (SUCCEEDED(pFileOpenDialog->GetResults(&pItems)))
            {
                DWORD index = 0;
                IShellItem* pItem;
                while (SUCCEEDED(pItems->GetItemAt(index++, &pItem)))
                {
                    PWSTR pName;
                    if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pName)))
                    {
                        items.push_back(pName);
                        CoTaskMemFree(pName);
                    }
                    pItem->Release();
                }
                pItems->Release();
            }
        }
        pFileOpenDialog->Release();
    }
    return items;
}

Optional<Pair<ComStr,INT>> PickIconDialog(HWND hWnd, StrView path, INT index)
{
    auto str = ComAllocStr(PATH_MAX - 1, path); // in/out
    if (PickIconDlg(hWnd, str.get(), PATH_MAX - 1, &index))
        return Pair(std::move(str), index);
    return std::nullopt;
}
