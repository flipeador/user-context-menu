#include "framework.h"

static bool s_init = false;

static void Initialize();

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
    if (s_init) return; s_init = true;

    auto installPath = Path(GetModuleFileName(g_hModule)).parent_path();

    auto packageLocalData = std::format(
        L"{}/Packages/{}/LocalState",
        GetKnownFolderPath(FOLDERID_LocalAppData),
        std::regex_replace(installPath.filename().wstring(), std::wregex(L"(_.*__)"), L"_")
    );

    SetEnvironmentVariable(L":LOCAL", packageLocalData);
    SetEnvironmentVariable(L":INSTALL", installPath.wstring());
    SetEnvironmentVariable(L":DESKTOP", GetKnownFolderPath(FOLDERID_Desktop));
}

STDAPI_(void) DllFindPath(const wchar_t* in, wchar_t* out)
{
    Initialize();
    auto path = FindPath(ExpandEnvironmentStrings(in));
    path.copy(out, path.size()); out[path.size()] = L'\0';
}

STDAPI_(HICON) DllExtractIcon(const wchar_t* path, int index)
{
    Initialize();
    return ExtractIconW(g_hModule, path, index);
}

STDAPI DllCanUnloadNow()
{
    return g_count ? S_FALSE : S_OK;
}

STDAPI DllGetClassObject(const CLSID& clsid, const IID& iid, PPV ppv)
{
    Initialize();
    COM_INIT_PPV_ARG(ppv);
    if (clsid != __uuidof(ExplorerCommand))
        return CLASS_E_CLASSNOTAVAILABLE;
    return ComCreateInterface<ClassFactory>(iid, ppv);
}
