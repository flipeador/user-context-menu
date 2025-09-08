#pragma once

#define COM_CHECK_ARG(p) if (!p) return E_POINTER;
#define COM_SET_ARG(p, v) COM_CHECK_ARG(p); *p = v;
#define COM_INIT_PPV_ARG(p) COM_SET_ARG(p, nullptr)

#define COM_RETURN_IF_FAILED(_) \
    {                           \
        auto hr = _;            \
        if (FAILED(hr))         \
            return hr;          \
    }

#define COM_CREATE_INSTANCE(i, c) \
    if (iid == __uuidof(i))       \
    {                             \
        *ppv = NEW_NOTHROW(c);    \
        if (*ppv == nullptr)      \
            return E_OUTOFMEMORY; \
        return S_OK;              \
    }

#define COM_DEFINE_IUNKNOWN_METHODS             \
    ULONG AddRef() override                     \
    {                                           \
        return SafeIncrement(m_count);          \
    }                                           \
    ULONG Release() override                    \
    {                                           \
        auto count = SafeDecrement(m_count);    \
        if (count == 0) delete this;            \
        return count;                           \
    }                                           \
    HRESULT QueryInterface(RIID, PPV) override;

#define COM_QUERY_SERVICE(punk, guid, ppv)                \
    IUnknown_QueryService(punk, guid, IID_PPV_ARGS(&ppv))

template <typename T>
concept C_IUnknown =
    requires(T t) {
        { t.AddRef() } -> std::same_as<ULONG>;
        { t.Release() } -> std::same_as<ULONG>;
};

template <C_IUnknown T>
inline auto SafeAddRef(T*& rpv)
{
    if (rpv != nullptr)
        rpv->AddRef();
}

template <C_IUnknown T>
inline auto SafeRelease(T*& rpv)
{
    IUnknown_AtomicRelease((PPV)&rpv);
}

struct CoTaskMemDeleter
{
    void operator()(void* p) const noexcept
    {
        CoTaskMemFree(p);
    }
};

using ComStr = std::unique_ptr<WCHAR[], CoTaskMemDeleter>;

inline auto ComAllocStr(size_t capacity, StrView str)
{
    auto ptr = (PWSTR)CoTaskMemAlloc((capacity + 1) * sizeof(WCHAR));
    if (ptr == nullptr) throw std::bad_alloc();
    auto count = str.copy(ptr, std::min(capacity, str.size()));
    ptr[count] = L'\0';
    return ComStr(ptr);
}

inline auto ComDupStr(std::string_view sv, PWSTR* ppwc)
{
    // Probably OK due to `app.manifest`:
    // <activeCodePage>UTF-8</activeCodePage>
    // (per-process overriding of system's default codepage)
    // 
    // Use Unicode UTF-8 for worldwide language support:
    // [HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Nls\CodePage]
    // "ACP"="65001"
    // "OEMCP"="65001"
    // "MACCP"="65001"
    // (global overriding of system's default codepage)
    //
    // Assuming `SHStrDupA` uses `MultiByteToWideChar` with `CP_ACP`:
    // Set the active code page to UTF-8 (65001) so `CP_ACP=CP_UTF8`.
    return SHStrDupA(sv.data(), ppwc); // CP_ACP/CP_UTF8 to UTF-16 (CoTaskMem)
}
