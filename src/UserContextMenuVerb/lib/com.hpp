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
    HRESULT QueryInterface(const IID&, PPV) override;

#define COM_SET_PPV_ARG(ppv, pv) \
    if (ppv == nullptr)          \
        return E_POINTER;        \
    *ppv = pv;

#define COM_INIT_PPV_ARG(ppv) COM_SET_PPV_ARG(ppv, nullptr)

#define COM_QUERY_SERVICE(punk, guid, ppv) \
    IUnknown_QueryService(punk, guid, IID_PPV_ARGS(&ppv))

template <typename T>
concept C_IUnknown =
    requires(T t) {
        { t.AddRef() } -> std::same_as<ULONG>;
        { t.Release() } -> std::same_as<ULONG>;
};

template <C_IUnknown T>
inline void SafeAddRef(T*& rpv)
{
    if (rpv != nullptr)
        rpv->AddRef();
}

template <C_IUnknown T>
inline void SafeRelease(T*& rpv)
{
    IUnknown_AtomicRelease((PPV)&rpv);
}

template<C_IUnknown T, typename... Args>
inline HRESULT ComCreateInterface(const IID& iid, PPV ppv, Args&&... args)
{
    T* pInterface = NEW_NOTHROW(T(std::forward<Args>(args)...));
    if (pInterface == nullptr) return E_OUTOFMEMORY;
    auto hr = pInterface->QueryInterface(iid, ppv);
    pInterface->Release();
    return hr;
}
