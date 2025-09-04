#pragma once

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

std::string MapStr(std::wstring_view wstr);
std::wstring MapStr(std::string_view str);

/***************************************************
 * MISC
***************************************************/

BOOL IsKeyDown(IList<INT> keys);
BOOL IsDarkThemeEnabled();
String GetShellItemPath(ITEMIDLIST* pIDL);
String GetKnownFolderPath(RIID iid);
String GetModulePath(HMODULE hModule);
String GetEnvironmentVariable(StrView name);
BOOL SetEnvironmentVariable(StrView name, const Optional<StrView>& value);
String ExpandEnvironmentStrings(StrView str);
Optional<DWORD> GetFileAttributes(StrView path);
Generator<String> ParseItems(StrView str, WCHAR sep);
String FindPath(Path name, DWORD mask = 0, DWORD attr = 0);
DWORD ShellExecute(HWND hWnd, StrView verb, StrView file, StrView args, StrView wdir, INT scmd);

/***************************************************
 * DIALOGS
***************************************************/

Optional<Pair<ComStr, INT>> PickIcon(HWND hWnd, StrView path, INT index);
