#include "framework.hpp"

ClassFactory::ClassFactory()
{
    SafeIncrement(g_count);
}

ClassFactory::~ClassFactory()
{
    SafeDecrement(g_count);
}

HRESULT ClassFactory::QueryInterface(RIID iid, PPV ppv)
{
    static const QITAB qit[] = {
        QITABENT(ClassFactory, IClassFactory),
        { 0 }
    };
    return QISearch(this, qit, iid, ppv);
}

HRESULT ClassFactory::CreateInstance(IUnknown* pnk, RIID iid, PPV ppv)
{
    COM_INIT_PPV_ARG(ppv);
    if (pnk) return CLASS_E_NOAGGREGATION;
    COM_CREATE_INSTANCE(IContextMenu, ContextMenu);
    COM_CREATE_INSTANCE(IExplorerCommand, ExplorerCommand);
    return E_NOINTERFACE;
}

HRESULT ClassFactory::LockServer(BOOL lock)
{
    if (lock) SafeIncrement(g_count);
    else SafeDecrement(g_count);
    return S_OK;
}
