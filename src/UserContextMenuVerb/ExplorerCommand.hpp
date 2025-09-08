#pragma once

class __declspec(uuid(PACKAGE_COM_CLSID)) ExplorerCommand;

class ExplorerCommand final
    : public IExplorerCommand
    , public IObjectWithSite
{
public:
    explicit ExplorerCommand(); // root command
    explicit ExplorerCommand(ExplorerCommand*, Json*); // subcommand
    ~ExplorerCommand();

    // IUnknown
    COM_DEFINE_IUNKNOWN_METHODS;

    // IExplorerCommand
    HRESULT GetTitle(IShellItemArray*, PWSTR*) override;
    HRESULT GetIcon(IShellItemArray*, PWSTR*) override;
    HRESULT GetToolTip(IShellItemArray*, PWSTR*) override;
    HRESULT GetCanonicalName(GUID*) override;
    HRESULT GetState(IShellItemArray*, BOOL, EXPCMDSTATE*) override;
    HRESULT Invoke(IShellItemArray*, IBindCtx*) override;
    HRESULT GetFlags(EXPCMDFLAGS*) override;
    HRESULT EnumSubCommands(IEnumExplorerCommand**) override;

    // IObjectWithSite
    HRESULT SetSite(IUnknown*) override;
    HRESULT GetSite(RIID, PPV) override;
private:
    Json* m_pJson{ };
    ExplorerCommand* m_pParent{ };
    std::shared_ptr<Command> m_cmd;
    // IUnknown
    LONG m_count = 1;
    // IObjectWithSite
    IUnknown* m_pSite{ };

    ExplorerCommand* GetRoot();
    BOOL ProcessCommand(Json& command);
    HRESULT Initialize(IShellItemArray* pItems);
};
