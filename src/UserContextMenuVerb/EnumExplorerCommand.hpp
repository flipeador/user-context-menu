#pragma once

class EnumExplorerCommand final : public IEnumExplorerCommand
{
public:
    explicit EnumExplorerCommand(ExplorerCommand*, Json*);
    ~EnumExplorerCommand();

    // IUnknown
    COM_DEFINE_IUNKNOWN_METHODS;

    // IEnumExplorerCommand
    HRESULT Clone(IEnumExplorerCommand**) override;
    HRESULT Next(ULONG, IExplorerCommand**, ULONG*) override;
    HRESULT Reset() override;
    HRESULT Skip(ULONG) override;
private:
    ExplorerCommand* m_pExpCmd;
    Json* m_pJson;
    // IUnknown
    LONG m_count = 1;
    // IEnumExplorerCommand
    size_t m_cursor = 0;
};
