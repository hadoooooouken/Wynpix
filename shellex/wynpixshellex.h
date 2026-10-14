// shellex/wynpixshellex.h
#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <shlobj.h>
#include <thumbcache.h>
#include <shlwapi.h>
#include <string>

// Unique CLSID for WynpixThumbnailProvider COM object
// CLSID: {9E9681D4-2454-445F-8C28-79AAF86F9F15}
extern const CLSID CLSID_WynpixThumbnailProvider;

class WynpixThumbnailProvider : public IThumbnailProvider, public IInitializeWithFile, public IInitializeWithStream {
public:
    WynpixThumbnailProvider();

    // IUnknown
    IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv) noexcept;
    IFACEMETHODIMP_(ULONG) AddRef() noexcept;
    IFACEMETHODIMP_(ULONG) Release() noexcept;

    // IInitializeWithFile
    IFACEMETHODIMP Initialize(LPCWSTR pszFilePath, DWORD grfMode) noexcept;

    // IInitializeWithStream
    IFACEMETHODIMP Initialize(IStream *pStream, DWORD grfMode) noexcept;

    // IThumbnailProvider
    IFACEMETHODIMP GetThumbnail(UINT cx, HBITMAP *phbmp,
                               WTS_ALPHATYPE *pdwAlpha) noexcept;

protected:
    virtual ~WynpixThumbnailProvider();

private:
    long m_cRef;
    std::wstring m_szFilePath;
    IStream* m_pStream;
};

class WynpixThumbnailProviderClassFactory : public IClassFactory {
public:
    WynpixThumbnailProviderClassFactory();

    // IUnknown
    IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv) noexcept;
    IFACEMETHODIMP_(ULONG) AddRef() noexcept;
    IFACEMETHODIMP_(ULONG) Release() noexcept;

    // IClassFactory
    IFACEMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid,
                                  void **ppv) noexcept;
    IFACEMETHODIMP LockServer(BOOL fLock) noexcept;

protected:
    virtual ~WynpixThumbnailProviderClassFactory();

private:
    long m_cRef;
};
