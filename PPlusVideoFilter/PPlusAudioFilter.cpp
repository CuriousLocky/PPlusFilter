#include "PPlusFilter.h"

PPlusAudio::PPlusAudio(IUnknown* unknown, HRESULT* resultPointer)
	: CSource(NAME("PPlusAudio"), unknown, CLSID_PPlusAudio) {
	streamPin = new PPlusAudioStream(resultPointer, this);

	if (resultPointer != NULL) {
		if (streamPin == NULL) {
			*resultPointer = E_OUTOFMEMORY;
		}
		else {
			*resultPointer = S_OK;
		}
	}
}

PPlusAudio::~PPlusAudio() {
	delete streamPin;
}

STDMETHODIMP_(HRESULT __stdcall) PPlusAudio::QueryInterface(REFIID riid, void** ppv) {
    //Forward request for IAMStreamConfig & IKsPropertySet to the pin
    if (riid == _uuidof(IAMStreamConfig) || riid == _uuidof(IKsPropertySet))
        return streamPin->QueryInterface(riid, ppv);
    else
        return CSource::QueryInterface(riid, ppv);
}

CUnknown* __stdcall PPlusAudio::CreateInstance(IUnknown* unknown, HRESULT* resultPointer)
{
	PPlusAudio* newFilter = new PPlusAudio(unknown, resultPointer);

	if (resultPointer != NULL) {
		if (newFilter == NULL)
			*resultPointer = E_OUTOFMEMORY;
		else
			*resultPointer = S_OK;
	}
	return newFilter;
}

STDMETHODIMP_(HRESULT __stdcall) PPlusAudioStream::QueryInterface(REFIID riid, void** ppv)
{
	// Standard OLE stuff
	if (riid == _uuidof(IAMStreamConfig))
		*ppv = (IAMStreamConfig*)this;
	else if (riid == _uuidof(IKsPropertySet))
		*ppv = (IKsPropertySet*)this;
	else
		return CSourceStream::QueryInterface(riid, ppv);

	AddRef();
	return S_OK;
}

HRESULT __stdcall PPlusAudioStream::SetFormat(AM_MEDIA_TYPE* mediaType)
{
	return E_NOTIMPL;
}

HRESULT __stdcall PPlusAudioStream::GetFormat(AM_MEDIA_TYPE** mediaTypePointer)
{
	return E_NOTIMPL;
}

HRESULT __stdcall PPlusAudioStream::GetNumberOfCapabilities(int* countPointer, int* sizePointer)
{
	return E_NOTIMPL;
}

HRESULT __stdcall PPlusAudioStream::GetStreamCaps(int index, AM_MEDIA_TYPE** mediaTypePointer, BYTE* pSCC)
{
	return E_NOTIMPL;
}

HRESULT __stdcall PPlusAudioStream::Set(REFGUID guidPropSet, DWORD dwID, void* pInstanceData, DWORD cbInstanceData, void* pPropData, DWORD cbPropData)
{
	return E_NOTIMPL;
}

HRESULT __stdcall PPlusAudioStream::Get(REFGUID guidPropSet, DWORD dwPropID, void* pInstanceData, DWORD cbInstanceData, void* pPropData, DWORD cbPropData, DWORD* pcbReturned)
{
	if (guidPropSet != AMPROPSETID_Pin)
		return E_PROP_SET_UNSUPPORTED;
	if (dwPropID != AMPROPERTY_PIN_CATEGORY)
		return E_PROP_ID_UNSUPPORTED;
	if (pPropData == NULL && pcbReturned == NULL)
		return E_POINTER;
	if (pcbReturned)
		*pcbReturned = sizeof(GUID);
	if (pPropData == NULL)  // Caller just wants to know the size.
		return S_OK;
	if (cbPropData < sizeof(GUID)) // The buffer is too small.
		return E_UNEXPECTED;
	*(GUID*)pPropData = PIN_CATEGORY_CAPTURE;
	return S_OK;
}

HRESULT __stdcall PPlusAudioStream::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD* pTypeSupport)
{
	return E_NOTIMPL;
}

PPlusAudioStream::PPlusAudioStream(HRESULT* resultPointer, PPlusAudio* parentFilter)
	: CSourceStream(PPLUSAUDIONAME, resultPointer, parentFilter, L"Out") {
}

PPlusAudioStream::~PPlusAudioStream()
{
}

HRESULT PPlusAudioStream::DecideBufferSize(IMemAllocator* allocator, ALLOCATOR_PROPERTIES* request)
{
	return E_NOTIMPL;
}

HRESULT PPlusAudioStream::FillBuffer(IMediaSample* pSample)
{
	return E_NOTIMPL;
}

HRESULT PPlusAudioStream::SetMediaType(const CMediaType* pMediaType)
{
	return E_NOTIMPL;
}

HRESULT PPlusAudioStream::CheckMediaType(const CMediaType* pMediaType)
{
	return E_NOTIMPL;
}

HRESULT PPlusAudioStream::GetMediaType(int iPosition, CMediaType* pmt)
{
	return E_NOTIMPL;
}