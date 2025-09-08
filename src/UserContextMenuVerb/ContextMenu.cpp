#include "framework.hpp"

ContextMenu::ContextMenu()
{
    SafeIncrement(g_count);
}

ContextMenu::~ContextMenu()
{
    SafeDecrement(g_count);
}

/***************************************************
 * IUnknown
***************************************************/

HRESULT ContextMenu::QueryInterface(RIID iid, PPV ppv)
{
    static const QITAB qit[] = {
        QITABENT(ContextMenu, IContextMenu),
        QITABENT(ContextMenu, IShellExtInit),
        { 0 },
    };
    return QISearch(this, qit, iid, ppv);
}

/***************************************************
 * IContextMenu
***************************************************/

HRESULT ContextMenu::QueryContextMenu(HMENU hMenu, UINT index, UINT id, UINT, UINT flags)
{
    auto startId = id; hMenu; index; flags;
    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(startId == id ? 0 : (id - startId) + 1));
}

HRESULT ContextMenu::GetCommandString(UINT_PTR, UINT, UINT*, CHAR*, UINT)
{
    return E_NOTIMPL;
}

HRESULT ContextMenu::InvokeCommand(CMINVOKECOMMANDINFO* pInfo)
{
    auto id = (INT_PTR)pInfo->lpVerb;
    if (!IS_INTRESOURCE(id)) return E_FAIL;
    return S_OK;
}

/***************************************************
 * IShellExtInit
***************************************************/

HRESULT ContextMenu::Initialize(LPCITEMIDLIST pFolder, IDataObject* pDataObject, HKEY)
{
    pFolder; pDataObject;
    return S_OK;
}
