#include "framework.hpp"

#define IS_COMMAND (m_pJson && m_pJson->is_object())

#define RETURN_HIDDEN_IF(_)            \
    if (_)                             \
    {                                  \
        command["state"] = ECS_HIDDEN; \
        return FALSE;                  \
    }

static auto GetCommandRegex(const Json& cmd, std::string_view key)
{
    std::unique_ptr<std::wregex> re;
    auto regex = JSON_GET_WSTR(cmd["regex"][key]);
    if (!regex.empty())
    {
        try
        {
            re = std::make_unique<std::wregex>(regex);
        }
        catch (const std::regex_error&) { /* ignore invalid regex */ };
    }
    return re;
}

ExplorerCommand::ExplorerCommand()
{
    m_cmd = std::make_shared<Command>();
    m_cmd->json = LoadPackageCommands();

    if (m_cmd->json.is_array() && m_cmd->json.size())
        m_cmd->json = m_cmd->json[0];

    m_pJson = &m_cmd->json;

    SafeIncrement(g_count);
}

ExplorerCommand::ExplorerCommand(ExplorerCommand* pParent, Json* pCommand)
    : m_pParent(pParent)
    , m_pJson(pCommand)
{
    SafeAddRef(m_pParent);
    SafeIncrement(g_count);
}

ExplorerCommand::~ExplorerCommand()
{
    SafeRelease(m_pSite);
    SafeRelease(m_pParent);
    SafeDecrement(g_count);
}

ExplorerCommand* ExplorerCommand::GetRoot()
{
    return m_pParent ? m_pParent->GetRoot() : this;
}

BOOL ExplorerCommand::ProcessCommand(Json& command)
{
    EXPCMDSTATE state = command["state"];
    RETURN_HIDDEN_IF(BITALL(state, ECS_HIDDEN));

    CommandType type = command["type"];

    // Unknown | Desktop | Background
    if (m_cmd->items.empty())
    {
        RETURN_HIDDEN_IF(
            m_cmd->background.empty() ? !BITALL(type, CommandType::Unknown)    :
            m_cmd->isDesktop          ? !BITALL(type, CommandType::Desktop)    :
                                        !BITALL(type, CommandType::Background)
        );
    }
    // File | Drive | Directory
    else
    {
        auto mode = JSON_GET_STR(command["multi"]["mode"]);
        RETURN_HIDDEN_IF(mode == "Off" && m_cmd->items.size() > 1);

        auto regexInclude = GetCommandRegex(command, "include");
        auto regexExclude = GetCommandRegex(command, "exclude");

        for (const auto& [item, attr] : m_cmd->items)
        {
            RETURN_HIDDEN_IF(
                item.size() == 3                       ? !BITALL(type, CommandType::Drive)     :
                BITALL(attr, FILE_ATTRIBUTE_DIRECTORY) ? !BITALL(type, CommandType::Directory) :
                                                         !BITALL(type, CommandType::File)
            );

            if (regexInclude || regexExclude)
            {
                const auto filename = Path(item).filename().wstring();
                RETURN_HIDDEN_IF(regexInclude && !std::regex_search(filename, *regexInclude));
                RETURN_HIDDEN_IF(regexExclude && std::regex_search(filename, *regexExclude));
            }
        }
    }

    EXPCMDFLAGS flags = command["flags"];

    if (BITALL(flags, ECF_HASSUBCOMMANDS))
    {
        size_t count = 0;
        for (auto& subcommand : command["children"])
            count += ProcessCommand(subcommand);
        RETURN_HIDDEN_IF(count == 0);
    }

    return TRUE;
}

HRESULT ExplorerCommand::Initialize(IShellItemArray* pItems)
{
    if (!IS_COMMAND) return S_FALSE;
    if (m_pParent != nullptr) return S_OK;
    if (m_pSite == nullptr) return S_FALSE;

    // Get the background item.
    IFolderView* pFolderView;
    if (SUCCEEDED(COM_QUERY_SERVICE(m_pSite, SID_SFolderView, pFolderView)))
    {
        IPersistFolder2* pPersistFolder2;
        if (SUCCEEDED(pFolderView->GetFolder(IID_PPV_ARGS(&pPersistFolder2))))
        {
            // Determine if the background is the actual desktop.
            // This is not the same as comparing the background path.
            CLSID clsid { };
            pPersistFolder2->GetClassID(&clsid);
            m_cmd->isDesktop = clsid == CLSID_ShellDesktop;

            // Get the background directory path as a string.
            // There is no background in locations like `This PC`.
            ITEMIDLIST* pIDL = nullptr;
            pPersistFolder2->GetCurFolder(&pIDL);
            m_cmd->background = GetShellItemIDListPath(pIDL);
            CoTaskMemFree(pIDL);

            pPersistFolder2->Release();
        }
        pFolderView->Release();
    }

    // Get the selected items.
    if (pItems != nullptr)
    {
        DWORD index = 0;
        IShellItem* pItem;

        // Iterate through all items and add them to the vector.
        while (SUCCEEDED(pItems->GetItemAt(index++, &pItem)))
        {
            PWSTR pName;
            if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pName)))
            {
                // Use `GetFileAttributes` instead of `IShellItem::GetAttributes`.
                // ZIP compressed files can be flagged with `SFGAO_FOLDER`.
                if (const auto attr = GetFileAttributes(pName); attr)
                    m_cmd->items.push_back({ pName, *attr });
                CoTaskMemFree(pName);
            }
            pItem->Release();
        }

        // Check if there are any items that could not be retrieved.
        if (FAILED(pItems->GetCount(&index)) || index != m_cmd->items.size())
            return S_FALSE;
    }

    return ProcessCommand(*m_pJson) ? S_OK : S_FALSE;
}

/***************************************************
 * IUnknown
***************************************************/

HRESULT ExplorerCommand::QueryInterface(RIID iid, PPV ppv)
{
    static const QITAB qit[] = {
        QITABENT(ExplorerCommand, IExplorerCommand),
        QITABENT(ExplorerCommand, IObjectWithSite),
        { 0 },
    };
    return QISearch(this, qit, iid, ppv);
}

/***************************************************
 * IExplorerCommand
***************************************************/

HRESULT ExplorerCommand::GetState(IShellItemArray* pItems, BOOL, EXPCMDSTATE* pState)
{
    if (FAILED(Initialize(pItems)))
    {
        *pState = ECS_HIDDEN;
    }
    else
    {
        m_pJson->at("state").get_to(*pState);
    }
    return S_OK;
}

HRESULT ExplorerCommand::GetTitle(IShellItemArray*, PWSTR* ppv)
{
    COM_INIT_PPV_ARG(ppv);
    if (!IS_COMMAND) return E_NOTIMPL;
    return ComDupStr(JSON_GET_STR(m_pJson->at("title")), ppv);
}

HRESULT ExplorerCommand::GetIcon(IShellItemArray*, PWSTR* ppv)
{
    COM_INIT_PPV_ARG(ppv);
    if (!IS_COMMAND) return E_NOTIMPL;
    size_t n = g_pInitObj->isDarkTheme;
    const auto& icons = m_pJson->at("icons");
    auto path = JSON_GET_WSTR(icons[n][0]);
    auto icon = FindFilePath(path);
    icon = std::format(L"\"{}\",{}", icon, (INT)icons[n][1]);
    return SHStrDupW(icon.data(), ppv); // "resource",index
}

HRESULT ExplorerCommand::GetFlags(EXPCMDFLAGS* pFlags)
{
    if (!IS_COMMAND) return E_NOTIMPL;
    m_pJson->at("flags").get_to(*pFlags);
    return S_OK;
}

HRESULT ExplorerCommand::GetToolTip(IShellItemArray*, PWSTR* ppv)
{
    COM_INIT_PPV_ARG(ppv);
    return E_NOTIMPL;
}

HRESULT ExplorerCommand::GetCanonicalName(GUID* ppv)
{
    COM_SET_ARG(ppv, __uuidof(this));
    return S_OK;
}

HRESULT ExplorerCommand::EnumSubCommands(IEnumExplorerCommand** ppEnumExpCmd)
{
    COM_INIT_PPV_ARG(ppEnumExpCmd);
    auto& subcommands = m_pJson->at("children");
    if (!subcommands.is_array()) return E_NOTIMPL;
    *ppEnumExpCmd = new EnumExplorerCommand(this, &subcommands);
    return S_OK;
}

HRESULT ExplorerCommand::Invoke(IShellItemArray*, IBindCtx*)
{
    HWND hWnd = nullptr;
    IUnknown_GetWindow(m_pSite, &hWnd);
    return GetRoot()->m_cmd->Invoke(hWnd, *m_pJson);
}

/***************************************************
 * IObjectWithSite
***************************************************/

/**
 * Called immediately only for the root command when the context menu is invoked.
 * If a command is invoked, it is also called for its associated interface instance.
 */
HRESULT ExplorerCommand::SetSite(IUnknown* pSite)
{
    // Release old, set + addref new.
    IUnknown_Set(&m_pSite, pSite);
    return S_OK;
}

HRESULT ExplorerCommand::GetSite(RIID iid, PPV ppv)
{
    COM_INIT_PPV_ARG(ppv);
    if (m_pSite == nullptr) return E_FAIL;
    return m_pSite->QueryInterface(iid, ppv);
}
