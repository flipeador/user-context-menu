#pragma once

// NOTE:
// The IExplorerCommand interface is for the modern context menu.
// The older context menu must be implemented with IContextMenu.

// Verb ID | COM Class ID (Package.appxmanifest)
class __declspec(uuid("4529C759-9140-4FF5-A577-C05357BA9508")) ExplorerCommand;

class ExplorerCommand final
    : public IExplorerCommand
    , public IObjectWithSite
{
public:
    enum class ItemType : uint32_t
    {
        // File | Directory
        File      = 0x00000001, // File
        Drive     = 0x00000002, // File
        Directory = 0x00000004, // Directory

        // Background (current folder)
        Unknown    = 0x00000008, // Background (This PC, Quick Access)
        Desktop    = 0x00000010, // Background
        Background = 0x00000020, // Background
    };

    explicit ExplorerCommand();
    explicit ExplorerCommand(ExplorerCommand*, Json*);
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
    HRESULT GetSite(const IID&, PPV) override;
private:
    struct InitObject
    {
        Json json;
        BOOL isDesktop;
        BOOL isDarkTheme;
        String background;
        std::vector<std::pair<String, SFGAOF>> items;
    };

    Json* m_pJson;
    ExplorerCommand* m_pParent;
    std::shared_ptr<InitObject> m_ctx;
    // IUnknown
    LONG m_count = 1;
    // IObjectWithSite
    IUnknown* m_pSite = nullptr;

    ExplorerCommand* GetRoot();
    BOOL ProcessCommand(Json& command);
    HRESULT Initialize(IShellItemArray* pItems);
};
