#pragma once
#include "common.h"
// {732BB97E-C0D8-4520-8E77-25C9A995BD4E}
DEFINE_GUID(CLSID_PPlusCamera ,
	0x732bb97e, 0xc0d8, 0x4520, 0x8e, 0x77, 0x25, 0xc9, 0xa9, 0x95, 0xbd, 0x4e);

class PPlusVideoStream;

class PPlusVideo : public CSource {

private:
    // Constructor is private because you have to use CreateInstance
    PPlusVideo(IUnknown* unknown, HRESULT* resultPointer);
    ~PPlusVideo();

    PPlusVideoStream* streamPin;

public:
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
    static CUnknown* WINAPI CreateInstance(IUnknown* unknown, HRESULT* resultPointer);
    IFilterGraph* GetGraph() { return m_pGraph; }
};

class PPlusVideoStream : public CSourceStream, public IAMStreamConfig, public IKsPropertySet {
protected:
    int m_iFrameNumber;
    const REFERENCE_TIME m_rtFrameLength;

    int m_iImageHeight;                 // The current image height
    int m_iImageWidth;                  // And current image width

    CMediaType m_MediaType;
    CCritSec m_cSharedState;            // Protects our internal state

    PPlusVideo* parentFilter;
    
    unsigned char* sharedBuffer;
    HANDLE sharedBufferFileHandle;

public:

    // For IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef() { return GetOwner()->AddRef(); }                                                          
    STDMETHODIMP_(ULONG) Release() { return GetOwner()->Release(); }

    // For IAMStreamConfig
    HRESULT STDMETHODCALLTYPE SetFormat(AM_MEDIA_TYPE* mediaType);
    HRESULT STDMETHODCALLTYPE GetFormat(AM_MEDIA_TYPE** mediaTypePointer);
    HRESULT STDMETHODCALLTYPE GetNumberOfCapabilities(int* countPointer, int* sizePointer);
    HRESULT STDMETHODCALLTYPE GetStreamCaps(int index, AM_MEDIA_TYPE** mediaTypePointer, BYTE* pSCC);

    // For IKsPropertySet
    HRESULT STDMETHODCALLTYPE Set(REFGUID guidPropSet, DWORD dwID, void* pInstanceData, DWORD cbInstanceData, void* pPropData, DWORD cbPropData);
    HRESULT STDMETHODCALLTYPE Get(REFGUID guidPropSet, DWORD dwPropID, void* pInstanceData, DWORD cbInstanceData, void* pPropData, DWORD cbPropData, DWORD* pcbReturned);
    HRESULT STDMETHODCALLTYPE QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD* pTypeSupport);

    PPlusVideoStream(HRESULT* resultPointer, PPlusVideo* parentFilter);
    ~PPlusVideoStream();

    // Override the version that offers exactly one media type
    HRESULT DecideBufferSize(IMemAllocator* allocator, ALLOCATOR_PROPERTIES* request);
    HRESULT FillBuffer(IMediaSample* pSample);

    // Set the agreed media type and set up the necessary parameters
    HRESULT SetMediaType(const CMediaType* pMediaType);

    // Support multiple display formats
    HRESULT CheckMediaType(const CMediaType* pMediaType);
    HRESULT GetMediaType(int iPosition, CMediaType* pmt);

    // Quality control
    // Not implemented because we aren't going in real time.
    // If the file-writing filter slows the graph down, we just do nothing, which means
    // wait until we're unblocked. No frames are ever dropped.
    STDMETHODIMP Notify(IBaseFilter* self, Quality quality) {
        return E_NOTIMPL;
    }

};
