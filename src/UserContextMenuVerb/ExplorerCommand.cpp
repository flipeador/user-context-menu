#include "framework.hpp"

#define IS_COMMAND (m_pJson && m_pJson->is_object())

#define RETURN_HIDDEN_IF(_)            \
    if (_)                             \
    {                                  \
        command["state"] = ECS_HIDDEN; \
        return FALSE;                  \
    }

static String FindPath2(StrView path, DWORD mask = 0, DWORD attr = 0)
{
    return FindPath(ExpandEnvironmentStrings(path), mask, attr);
}

static String ExpandCommandVars(Path path, StrView str)
{
    SetEnvironmentVariable(L":PARENT", path.parent_path().wstring());
    SetEnvironmentVariable(L":PATH", path.wstring());
    SetEnvironmentVariable(L":NAME", path.filename().wstring());
    SetEnvironmentVariable(L":STEM", path.stem().wstring());
    SetEnvironmentVariable(L":EXT", path.extension().wstring());
    return ExpandEnvironmentStrings(STR_IF_EMPTY(str, L"\"%:PATH%\""));
}

static void ExecCommand(HWND hWnd, StrView verb, StrView file, StrView cmdl, StrView args, StrView wdir, INT scmd)
{
    SetEnvironmentVariable(L":PATH", args);
    auto cmdl2 = ExpandEnvironmentStrings(STR_IF_EMPTY(cmdl, L"%:PATH%"));
    auto wdir2 = ExpandEnvironmentStrings(STR_IF_EMPTY(wdir, L"%:BACKGROUND%"));
    ShellExecute(hWnd, verb, file, cmdl2, wdir2, scmd);
}

static auto GetCommandIcon(const Json& cmd, size_t n)
{
    auto path = JSON_GET_WSTR(cmd["icons"][n][0]);
    auto icon = FindPath2(path, FILE_ATTRIBUTE_DIRECTORY);
    return std::format(L"\"{}\",{}", icon, (int)cmd["icons"][n][1]);
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
    : m_pParent(nullptr)
{
    m_ctx = std::make_shared<InitObject>();
    m_ctx->json = LoadPackageCommands();
    m_ctx->isDarkTheme = IsDarkThemeEnabled();

    if (m_ctx->json.is_array() && m_ctx->json.size())
        m_ctx->json = m_ctx->json[0];

    m_pJson = &m_ctx->json;

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

HRESULT ExplorerCommand::QueryInterface(RIID iid, PPV ppv)
{
    static const QITAB qit[] = {
        QITABENT(ExplorerCommand, IExplorerCommand),
        QITABENT(ExplorerCommand, IObjectWithSite),
        { 0 },
    };
    return QISearch(this, qit, iid, ppv);
}

HRESULT ExplorerCommand::GetTitle(IShellItemArray*, PWSTR* ppv)
{
    COM_INIT_PPV_ARG(ppv);
    if (!IS_COMMAND) return E_NOTIMPL;
    auto title = JSON_GET_STR(m_pJson->at("title"));
    return SHStrDupA(title.data(), ppv);
}

HRESULT ExplorerCommand::GetIcon(IShellItemArray*, PWSTR* ppv)
{
    COM_INIT_PPV_ARG(ppv);
    if (!IS_COMMAND) return E_NOTIMPL;
    size_t n = GetRoot()->m_ctx->isDarkTheme;
    const auto icon = GetCommandIcon(*m_pJson, n);
    return SHStrDupW(icon.data(), ppv);
}

HRESULT ExplorerCommand::GetToolTip(IShellItemArray*, PWSTR* ppv)
{
    COM_INIT_PPV_ARG(ppv);
    return E_NOTIMPL;
}

HRESULT ExplorerCommand::GetCanonicalName(GUID* pGuid)
{
    COM_SET_ARG(pGuid, __uuidof(this));
    return S_OK;
}

HRESULT ExplorerCommand::GetState(IShellItemArray* pItems, BOOL, EXPCMDSTATE* pState)
{
    if (FAILED(Initialize(pItems)))
    {
        *pState = ECS_HIDDEN;
        return S_OK;
    }

    m_pJson->at("state").get_to(*pState);
    return S_OK;
}

HRESULT ExplorerCommand::Invoke(IShellItemArray*, IBindCtx*)
{
    const auto& cmd = *m_pJson;
    const auto& ctx = *GetRoot()->m_ctx;

    HWND hWnd = nullptr;
    IUnknown_GetWindow(m_pSite, &hWnd);

    String verb = L"runas"; // elevated
    if (!IsKeyDown({ VK_CONTROL, VK_SHIFT }))
        verb = JSON_GET_WSTR(cmd["verb"]);

    INT scmd = cmd["scmd"];
    auto file = JSON_GET_WSTR(cmd["file"]);
    auto cmdl = JSON_GET_WSTR(cmd["args"]);
    auto wdir = JSON_GET_WSTR(cmd["wdir"]);
    auto mode = JSON_GET_STR(cmd["multi"]["mode"]);

    SetEnvironmentVariable(L":BACKGROUND", ctx.background);

    file = FindPath2(file);

    if (mode == "Off")
    {
        const auto& path = ctx.items.empty() ? ctx.background : ctx.items[0].first;
        ExecCommand(hWnd, verb, file, L"", ExpandCommandVars(path, cmdl), wdir, scmd);
    }
    else if (mode == "Each" || mode == "Join")
    {
        auto args = JSON_GET_WSTR(cmd["multi"]["args"]);

        if (ctx.items.empty())
        {
            const auto args2 = ExpandCommandVars(ctx.background, args);
            ExecCommand(hWnd, verb, file, cmdl, args2, wdir, scmd);
        }
        else if (mode == "Each")
        {
            for (const auto& [item, attr] : ctx.items)
            {
                const auto args2 = ExpandCommandVars(item, args);
                ExecCommand(hWnd, verb, file, cmdl, args2, wdir, scmd);
            }
        }
        else if (mode == "Join")
        {
            if (wdir.empty() && args.empty())
                args = L"\"%:NAME%\"";

            String args2;
            for (const auto& [item, attr] : ctx.items)
                args2 += ExpandCommandVars(item, args) + L" ";
            ExecCommand(hWnd, verb, file, cmdl, args2, wdir, scmd);
        }
    }

    return S_OK;
}

HRESULT ExplorerCommand::GetFlags(EXPCMDFLAGS* pFlags)
{
    if (!IS_COMMAND) return E_NOTIMPL;
    m_pJson->at("flags").get_to(*pFlags);
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

ExplorerCommand* ExplorerCommand::GetRoot()
{
    return m_pParent ? m_pParent->GetRoot() : this;
}

BOOL ExplorerCommand::ProcessCommand(Json& command)
{
    EXPCMDSTATE state = command["state"];
    RETURN_HIDDEN_IF(BITALL(state, ECS_HIDDEN));

    ItemType type = command["type"];

    bool okFile = BITALL(type, ItemType::File);
    bool okDrive = BITALL(type, ItemType::Drive);
    bool okDirectory = BITALL(type, ItemType::Directory);

    bool okUnknown = BITALL(type, ItemType::Unknown);
    bool okDesktop = BITALL(type, ItemType::Desktop);
    bool okBackground = BITALL(type, ItemType::Background);

    // Unknown | Desktop | Background
    if (m_ctx->items.empty())
    {
        RETURN_HIDDEN_IF(
            m_ctx->background.empty() ? !okUnknown :
            m_ctx->isDesktop ? !okDesktop :
            !okBackground
        );
    }
    // File | Drive | Directory
    else
    {
        auto mode = JSON_GET_STR(command["multi"]["mode"]);
        RETURN_HIDDEN_IF(mode == "Off" && m_ctx->items.size() > 1);

        auto regexInclude = GetCommandRegex(command, "include");
        auto regexExclude = GetCommandRegex(command, "exclude");

        for (const auto& [item, attr] : m_ctx->items)
        {
            RETURN_HIDDEN_IF(
                item.size() == 3 ? !okDrive :
                BITALL(attr, FILE_ATTRIBUTE_DIRECTORY) ? !okDirectory :
                !okFile
            );

            if (regexInclude || regexExclude)
            {
                const auto& filename = Path(item).filename().wstring();
                RETURN_HIDDEN_IF(regexInclude && !std::regex_search(filename, *regexInclude));
                RETURN_HIDDEN_IF(regexExclude && std::regex_search(filename, *regexExclude));
            }
        }
    }

    EXPCMDFLAGS flags = command.at("flags");

    if (BITALL(flags, ECF_HASSUBCOMMANDS))
    {
        size_t count = 0;
        for (auto& subcommand : command.at("children"))
            if (subcommand.is_object())
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
            m_ctx->isDesktop = clsid == CLSID_ShellDesktop;

            // Get the background directory path as a string.
            // There is no background in locations like `This PC`.
            ITEMIDLIST* pIDL = nullptr;
            pPersistFolder2->GetCurFolder(&pIDL);
            m_ctx->background = GetShellItemPath(pIDL);
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
                    m_ctx->items.push_back({ pName, *attr });

                CoTaskMemFree(pName);
            }

            pItem->Release();
        }

        // Check if there are any items that could not be retrieved.
        if (FAILED(pItems->GetCount(&index)) || index != m_ctx->items.size())
            return S_FALSE;
    }

    return ProcessCommand(*m_pJson) ? S_OK : S_FALSE;
}
