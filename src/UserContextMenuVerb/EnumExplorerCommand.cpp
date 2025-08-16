#include "framework.h"

EnumExplorerCommand::EnumExplorerCommand(ExplorerCommand* pExpCmd, Json* pJson)
    : m_pExpCmd(pExpCmd)
    , m_pJson(pJson)
{
    SafeAddRef(m_pExpCmd);
}

EnumExplorerCommand::~EnumExplorerCommand()
{
    SafeRelease(m_pExpCmd);
}

HRESULT EnumExplorerCommand::QueryInterface(const IID& iid, PPV ppv)
{
    static const QITAB qit[] = {
        QITABENT(EnumExplorerCommand, IEnumExplorerCommand),
        { 0 },
    };
    return QISearch(this, qit, iid, ppv);
}

HRESULT EnumExplorerCommand::Clone(IEnumExplorerCommand**)
{
    return E_NOTIMPL;
}

HRESULT EnumExplorerCommand::Next(ULONG count, IExplorerCommand** pExpCmd, ULONG* pFetched)
{
    ULONG index = 0;
    while (index < count && m_cursor < m_pJson->size())
    {
        auto& command = m_pJson->at(m_cursor++);
        pExpCmd[index++] = new ExplorerCommand(m_pExpCmd, &command);
    }
    SafeAssign(pFetched, index);
    return index ? S_OK : S_FALSE;
}

HRESULT EnumExplorerCommand::Reset()
{
    m_cursor = 0;
    return S_OK;
}

HRESULT EnumExplorerCommand::Skip(ULONG count)
{
    m_cursor += count;
    return S_OK;
}
