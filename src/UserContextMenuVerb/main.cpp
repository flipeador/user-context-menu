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

EXPORT VOID DllInit()
{
    if (g_pInitObj) return;

    g_pInitObj = new DllInitObject {
        .hWnd = FindWindowW(nullptr, L"User Context Menu | Flipeador"),
        .isDarkTheme = GetSystemColorScheme()
    };

    const auto desktopPath = GetKnownFolderPath(FOLDERID_Desktop);
    const auto localAppDataPath = GetKnownFolderPath(FOLDERID_LocalAppData);

    const auto installPath = Path(GetModulePath(g_hModule)).parent_path().wstring();
    const auto localDataPath = std::format(L"{}/Packages/{}/LocalState", localAppDataPath, PACKAGE_NAME);

    SetEnvironmentVariable(L":LOCAL", localDataPath);
    SetEnvironmentVariable(L":INSTALL", installPath);
    SetEnvironmentVariable(L":DESKTOP", desktopPath);
}

EXPORT VOID DllFindPath(PCWSTR pwc, PWSTR* ppwc)
{
    ComDupStr(FindFilePath(pwc), ppwc);
}

EXPORT INT DllPickIcon(HWND hWnd, INT index, PCWSTR pwc, PWSTR* ppwc)
{
    if (auto icon = PickIconDialog(hWnd, pwc, index))
    {
        *ppwc = icon->first.release();
        return icon->second; // index
    }
    return 0;
}

EXPORT HICON DllExtractIcon(PCWSTR path, INT index)
{
    HICON hIcon = nullptr;
    SHDefExtractIconW(path, index, 0, &hIcon, nullptr, 256);
    return hIcon;
}

/***************************************************
 * PRIVATE EXPORT: COM SERVER
***************************************************/

EXPORT HRESULT DllCanUnloadNow()
{
    SEND_DEBUG_MESSAGE(L"DllCanUnloadNow:\n\tRefCount={}", g_count);

    return g_count == 0 ? S_OK : S_FALSE;
}

EXPORT HRESULT DllGetClassObject(RCLSID clsid, RIID iid, PPV ppv)
{
    DllInit();

    SEND_DEBUG_MESSAGE(
        L"DllGetClassObject:\n\tRefCount={}\n\tCLSID={}\n\tIID={}\n\tPPV={}",
        g_count, StringFromGUID(clsid), StringFromGUID(iid), (PVOID)ppv
    );

    COM_INIT_PPV_ARG(ppv);
    if (clsid != __uuidof(ExplorerCommand))
        return CLASS_E_CLASSNOTAVAILABLE;
    COM_CREATE_INSTANCE(IClassFactory, ClassFactory);
    return E_NOINTERFACE;
}

// No manual COM registration (DllRegisterServer and DllUnregisterServer):
// The shell extension (un)registration is managed by the WinUI app packaging system.
