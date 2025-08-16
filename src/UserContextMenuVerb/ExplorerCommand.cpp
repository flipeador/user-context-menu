#include "framework.h"

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
    SetEnvironmentVariable(L":PARENT", String(path.parent_path()));
    SetEnvironmentVariable(L":PATH", String(path));
    SetEnvironmentVariable(L":NAME", String(path.filename()));
    SetEnvironmentVariable(L":STEM", String(path.stem()));
    SetEnvironmentVariable(L":EXT", String(path.extension()));
    return ExpandEnvironmentStrings(STR_EMPTY(str, L"\"%:PATH%\""));
}

static DWORD ExecCommand(HWND hWnd, StrView verb, StrView file, StrView args, StrView margs, StrView wdir, INT scmd)
{
    SetEnvironmentVariable(L":PATH", margs);
    auto args2 = ExpandEnvironmentStrings(STR_EMPTY(args, L"%:PATH%"));
    auto wdir2 = ExpandEnvironmentStrings(STR_EMPTY(wdir, L"%:BACKGROUND%"));
    return ShellExecute(hWnd, verb, file, args2, wdir2, scmd);
}

static auto GetCommandIcon(const Json& cmd, size_t n)
{
    auto& path = cmd["icons"][n][0].get_ref<const JsonStr&>();
    auto icon = FindPath2(MapStr(path), FILE_ATTRIBUTE_DIRECTORY);
    return std::format(L"\"{}\",{}", icon, (int)cmd["icons"][n][1]);
}

static auto GetCommandRegex(const Json& cmd, std::string_view key)
{
    std::unique_ptr<std::wregex> re;
    auto& regex = cmd["regex"][key].get_ref<const JsonStr&>();
    if (!regex.empty())
    {
        try
        {
            re = std::make_unique<std::wregex>(MapStr(regex));
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

HRESULT ExplorerCommand::QueryInterface(const IID& iid, PPV ppv)
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
    auto& title = m_pJson->at("title").get_ref<JsonStr&>();
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
    COM_SET_PPV_ARG(pGuid, __uuidof(this));
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
    auto& cmd = *m_pJson;
    auto& ctx = *GetRoot()->m_ctx;

    HWND hWnd = nullptr;
    IUnknown_GetWindow(m_pSite, &hWnd);

    String verb = L"runas"; // Run as Administrator.
    auto elevated = IsKeyDown(VK_CONTROL) && IsKeyDown(VK_SHIFT);
    if (!elevated) verb = MapStr(cmd["verb"].get_ref<JsonStr&>());

    INT scmd = cmd["scmd"];
    auto file = MapStr(cmd["file"].get_ref<JsonStr&>());
    auto args = MapStr(cmd["args"].get_ref<JsonStr&>());
    auto wdir = MapStr(cmd["wdir"].get_ref<JsonStr&>());
    auto& multiMode = cmd["multi"]["mode"].get_ref<JsonStr&>();

    SetEnvironmentVariable(L":BACKGROUND", ctx.background);

    file = FindPath2(file);

    if (multiMode == "Off")
    {
        const auto& path = ctx.items.empty() ? ctx.background : ctx.items[0].first;
        ExecCommand(hWnd, verb, file, L"", ExpandCommandVars(path, args), wdir, scmd);
    }
    else if (multiMode == "Each" || multiMode == "Join")
    {
        auto multiArgs = MapStr(cmd["multi"]["args"].get_ref<JsonStr&>());

        if (ctx.items.empty())
        {
            const auto margs = ExpandCommandVars(ctx.background, multiArgs);
            ExecCommand(hWnd, verb, file, args, margs, wdir, scmd);
        }
        else if (multiMode == "Each")
        {
            for (const auto& [item, attr] : ctx.items)
            {
                const auto margs = ExpandCommandVars(item, multiArgs);
                ExecCommand(hWnd, verb, file, args, margs, wdir, scmd);
            }
        }
        else if (multiMode == "Join")
        {
            String margs;
            for (const auto& [item, attr] : ctx.items)
                margs += ExpandCommandVars(item, multiArgs) + L" ";
            ExecCommand(hWnd, verb, file, args, margs, wdir, scmd);
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

HRESULT ExplorerCommand::GetSite(const IID& iid, PPV ppv)
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
        auto& multiMode = command["multi"]["mode"].get_ref<JsonStr&>();
        RETURN_HIDDEN_IF(multiMode == "Off" && m_ctx->items.size() > 1);

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
            m_ctx->background = GetPathFromIDList(pIDL);
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
                //SFGAOF attr;
                //pItem->GetAttributes(SFGAO_FOLDER, &attr);

                // Use "GetFileAttributesW" instead of "GetAttributes".
                // ZIP compressed files can be flagged with "SFGAO_FOLDER".
                auto attr = GetFileAttributesW(pName);

                m_ctx->items.push_back({ pName, attr });
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
