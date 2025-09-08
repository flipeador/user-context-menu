#pragma once

#include "framework.hpp"

class __declspec(uuid(PACKAGE_COM_CLSID)) ContextMenu;

class ContextMenu final
    : public IContextMenu
    , public IShellExtInit
{
public:
    explicit ContextMenu();
    ~ContextMenu();

    // IUnknown
    COM_DEFINE_IUNKNOWN_METHODS;

    // IContextMenu
    HRESULT QueryContextMenu(HMENU, UINT, UINT, UINT, UINT) override;
    HRESULT InvokeCommand(CMINVOKECOMMANDINFO*) override;
    HRESULT GetCommandString(UINT_PTR, UINT, UINT*, CHAR*, UINT) override;

    // IShellExtInit
    HRESULT Initialize(LPCITEMIDLIST, IDataObject*, HKEY) override;
private:
    // IUnknown
    LONG m_count = 1;
};
