#pragma once

#define COM_DEFINE_IUNKNOWN_METHODS                   \
    ULONG AddRef() override                           \
    {                                                 \
        return SafeIncrement(m_count);                \
    }                                                 \
    ULONG Release() override                          \
    {                                                 \
        auto count = SafeDecrement(m_count);          \
        if (count == 0) delete this;                  \
        return count;                                 \
    }                                                 \
    HRESULT QueryInterface(RIID, PPV) override;

#define COM_CHECK_ARG(p) if (!p) return E_POINTER;
#define COM_SET_ARG(p, v) COM_CHECK_ARG(p); *p = v;
#define COM_INIT_PPV_ARG(p) COM_SET_ARG(p, nullptr)

#define COM_QUERY_SERVICE(punk, guid, ppv) \
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

template<C_IUnknown T, typename... Args>
inline HRESULT ComCreateInterface(RIID iid, PPV ppv, Args&&... args)
{
    T* pInterface = NEW_NOTHROW(T(std::forward<Args>(args)...));
    if (pInterface == nullptr) return E_OUTOFMEMORY;
    auto hr = pInterface->QueryInterface(iid, ppv);
    pInterface->Release();
    return hr;
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
    ptr[str.copy(ptr, std::min(capacity, str.size()))] = L'\0';
    return ComStr(ptr);
}
