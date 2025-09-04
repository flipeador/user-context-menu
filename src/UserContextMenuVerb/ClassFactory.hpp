#pragma once

class ClassFactory final : public IClassFactory
{
public:
    explicit ClassFactory();
    ~ClassFactory();

    // IUnknown
    COM_DEFINE_IUNKNOWN_METHODS;

    // IClassFactory
    HRESULT CreateInstance(IUnknown*, RIID, PPV) override;
    HRESULT LockServer(BOOL) override;
private:
    // IUnknown
    LONG m_count = 1;
};
