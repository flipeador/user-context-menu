#include "framework.h"

ClassFactory::ClassFactory()
{
    SafeIncrement(g_count);
}

ClassFactory::~ClassFactory()
{
    SafeDecrement(g_count);
}

HRESULT ClassFactory::QueryInterface(const IID& iid, PPV ppv)
{
    static const QITAB qit[] = {
        QITABENT(ClassFactory, IClassFactory),
        { 0 }
    };
    return QISearch(this, qit, iid, ppv);
}

HRESULT ClassFactory::CreateInstance(IUnknown* pnk, const IID& iid, PPV ppv)
{
    COM_INIT_PPV_ARG(ppv);
    if (pnk) return CLASS_E_NOAGGREGATION;
    return ComCreateInterface<ExplorerCommand>(iid, ppv);
}

HRESULT ClassFactory::LockServer(BOOL lock)
{
    if (lock) SafeIncrement(g_count);
    else SafeDecrement(g_count);
    return S_OK;
}
