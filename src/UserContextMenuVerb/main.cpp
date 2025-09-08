#include "framework.hpp"

ULONG g_count = 0;
HMODULE g_hModule;
DllInitObject* g_pInitObj = nullptr;

BOOL DllMain(HMODULE hModule, DWORD reason, PVOID)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);

        // Use this module handle instead of calling the `GetModuleHandle` function.
        // `GetModuleHandle` returns the handle of the module that loaded this file.
        g_hModule = hModule;
        break;
    case DLL_PROCESS_DETACH:
        delete g_pInitObj;
        break;
    }
    return TRUE;
}

/***************************************************
 * EXPORT
***************************************************/

EXTERN void DllInit()
{
    if (g_pInitObj) return;

    g_pInitObj = new DllInitObject { .isDarkTheme = IsDarkThemeEnabled() };

    const auto desktopPath = GetKnownFolderPath(FOLDERID_Desktop);
    const auto localAppDataPath = GetKnownFolderPath(FOLDERID_LocalAppData);

    const auto installPath = Path(GetModulePath(g_hModule)).parent_path().wstring();
    const auto localDataPath = std::format(L"{}/Packages/{}/LocalState", localAppDataPath, PACKAGE_NAME);

    SetEnvironmentVariable(L":LOCAL", localDataPath);
    SetEnvironmentVariable(L":INSTALL", installPath);
    SetEnvironmentVariable(L":DESKTOP", desktopPath);
}

EXTERN void DllFindPath(PCWSTR pwc, PWSTR* ppwc)
{
    SHStrDupW(FindFilePath(pwc).data(), ppwc);
}

EXTERN INT DllPickIcon(HWND hWnd, INT index, PCWSTR pwc, PWSTR* ppwc)
{
    if (auto icon = PickIcon(hWnd, pwc, index))
    {
        *ppwc = icon->first.release();
        return icon->second; // index
    }
    return 0;
}

EXTERN HICON DllExtractIcon(PCWSTR path, INT index)
{
    HICON hIcon = nullptr;
    SHDefExtractIconW(path, index, 0, &hIcon, nullptr, 256);
    return hIcon;
}

/***************************************************
 * COM SERVER PRIVATE EXPORT
***************************************************/

EXPORT DllCanUnloadNow()
{
    return g_count ? S_FALSE : S_OK;
}

EXPORT DllGetClassObject(RIID clsid, RIID iid, PPV ppv)
{
    DllInit();
    COM_INIT_PPV_ARG(ppv);
    if (clsid != __uuidof(ExplorerCommand))
        return CLASS_E_CLASSNOTAVAILABLE;
    COM_CREATE_INSTANCE(IClassFactory, ClassFactory);
    return E_NOINTERFACE;
}
