#include "framework.hpp"

static volatile bool g_init = false;

ULONG g_count = 0;
HMODULE g_hModule;

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
        break;
    }
    return TRUE;
}

static void Initialize()
{
    if (g_init) return; g_init = true;

    const auto desktopPath = GetKnownFolderPath(FOLDERID_Desktop);
    const auto localAppDataPath = GetKnownFolderPath(FOLDERID_LocalAppData);

    const auto installPath = Path(GetModulePath(g_hModule)).parent_path().wstring();
    const auto localDataPath = std::format(L"{}/Packages/{}/LocalState", localAppDataPath, PACKAGE_NAME);

    SetEnvironmentVariable(L":LOCAL", localDataPath);
    SetEnvironmentVariable(L":INSTALL", installPath);
    SetEnvironmentVariable(L":DESKTOP", desktopPath);
}

/***************************************************
 * EXPORT
***************************************************/

EXTERN void DllFindPath(PCWSTR pv, PWSTR* ppv)
{
    Initialize();
    const auto path = FindPath(ExpandEnvironmentStrings(pv));
    SHStrDupW(path.data(), ppv);
}

EXTERN INT DllPickIcon(HWND hWnd, INT index, PCWSTR pc, PWSTR* ppc)
{
    if (auto icon = PickIcon(hWnd, pc, index); icon)
    {
        *ppc = icon->first.release();
        return icon->second;
    }
    return 0;
}

EXTERN HICON DllExtractIcon(PCWSTR path, INT index)
{
    Initialize();
    HICON hIcon = nullptr;
    SHDefExtractIconW(path, index, 0, &hIcon, nullptr, 256);
    return hIcon;
}

/***************************************************
 * COM PRIVATE EXPORT
***************************************************/

EXPORT DllCanUnloadNow()
{
    return g_count ? S_FALSE : S_OK;
}

EXPORT DllGetClassObject(RIID clsid, RIID iid, PPV ppv)
{
    Initialize();
    COM_INIT_PPV_ARG(ppv);
    if (clsid != __uuidof(ExplorerCommand))
        return CLASS_E_CLASSNOTAVAILABLE;
    return ComCreateInterface<ClassFactory>(iid, ppv);
}
