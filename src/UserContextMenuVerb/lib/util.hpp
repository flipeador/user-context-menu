#pragma once

#define SEND_DEBUG_MESSAGE(...)                                                                               \
    if (g_pInitObj->hWnd)                                                                                     \
    {                                                                                                         \
        const auto str = std::format(__VA_ARGS__);                                                            \
        COPYDATASTRUCT cd { 0, (DWORD)str.size() * sizeof(wchar_t), (PVOID)str.data() };                      \
        const auto r = SendMessage(g_pInitObj->hWnd, WM_COPYDATA, NULL, (LPARAM)&cd, 2500, SMTO_ABORTIFHUNG); \
        if (!r || !*r) g_pInitObj->hWnd = nullptr;                                                            \
    }

template<typename T>
concept C_LongLike =
    std::is_integral_v<T> &&
    sizeof(T) == sizeof(long);

template<C_LongLike T>
inline T SafeIncrement(volatile T& l)
{
    return (T)_InterlockedIncrement((volatile long*)&l);
}

template<C_LongLike T>
inline T SafeDecrement(volatile T& l)
{
    return (T)_InterlockedDecrement((volatile long*)&l);
}

template <typename T>
inline void SafeAssign(T*& rpv, T value)
{
    if (rpv != nullptr)
        *rpv = value;
}

/***************************************************
 * STRING
***************************************************/

String MapStr(std::string_view str);
bool StrReplace(String& str, const std::wregex& pattern, const Function<Optional<String>(const std::wsmatch&)>& fn);
bool StrReplace(String& dest, const String& src, const std::wregex& pattern, const Function<Optional<String>(const std::wsmatch&)>& fn);

/***************************************************
 * MISC
***************************************************/

BOOL IsKeyDown(IList<INT> keys);
INT GetSystemColorScheme();
String GetModulePath(HMODULE hModule);
String GetEnvironmentVariable(StrView name);
BOOL SetEnvironmentVariable(StrView name, const Optional<StrView>& value);
String ExpandEnvironmentStrings(StrView str);
Optional<DWORD> GetFileAttributes(StrView path);
Generator<String> ParseItems(StrView str, WCHAR sep);
String FindPath(Path name, DWORD attm = 0, DWORD attv = 0);
String StringFromGUID(RGUID id);
String GetKnownFolderPath(RGUID id);
String GetItemIDListPath(LPCITEMIDLIST pIDL);
HANDLE ShellExecute(HWND hWnd, StrView verb, StrView file, StrView args, StrView wdir, INT scmd, UINT flags);
//Optional<String> DragQueryFile(HDROP hDrop, UINT index);

/***************************************************
 * MESSAGES
***************************************************/

Optional<LRESULT> SendMessage(HWND hWnd, UINT msg, WPARAM wParam = 0,
    LPARAM lParam = 0, UINT timeout = INFINITE, UINT flags = SMTO_NORMAL);

/***************************************************
 * DIALOGS
***************************************************/

String FileSaveDialog(const Function<HRESULT(IFileSaveDialog&)>& fn);
Vector<String> FileOpenDialog(const Function<HRESULT(IFileOpenDialog&)>& fn);
Optional<Pair<ComStr,INT>> PickIconDialog(HWND hWnd, StrView path, INT index);
