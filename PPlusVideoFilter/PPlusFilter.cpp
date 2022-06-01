#include "PPlusFilter.h"

PPlusVideo::PPlusVideo(IUnknown* unknown, HRESULT* resultPointer)
	: CSource(NAME("PPlusCamera"), unknown, CLSID_PPlusCamera) {
	streamPin = new PPlusVideoStream(resultPointer, this);

	if (resultPointer != NULL) {
		if (streamPin == NULL) {
			*resultPointer = E_OUTOFMEMORY;
		} else {
			*resultPointer = S_OK;
		}
	}
}

PPlusVideo::~PPlusVideo() {
	delete streamPin;
}

STDMETHODIMP_(HRESULT __stdcall) PPlusVideo::QueryInterface(REFIID riid, void** ppv) {
    //Forward request for IAMStreamConfig & IKsPropertySet to the pin
    if (riid == _uuidof(IAMStreamConfig) || riid == _uuidof(IKsPropertySet))
        return streamPin->QueryInterface(riid, ppv);
    else
        return CSource::QueryInterface(riid, ppv);
}

CUnknown* __stdcall PPlusVideo::CreateInstance(IUnknown* unknown, HRESULT* resultPointer) {
	PPlusVideo* newFilter = new PPlusVideo(unknown, resultPointer);

	if (resultPointer != NULL) {
		if (newFilter == NULL)
			*resultPointer = E_OUTOFMEMORY;
		else
			*resultPointer = S_OK;
	}
	return newFilter;
}

PPlusVideoStream::PPlusVideoStream(HRESULT* resultPointer, PPlusVideo* parentFilter)
    : CSourceStream(PPLUSCAMERANAME, resultPointer, parentFilter, L"Out"),
    parentFilter(parentFilter),
    m_iImageHeight(VIDEOHEIGHT),
    m_iImageWidth(VIDEOWIDTH),
    m_rtFrameLength(50000000) {
    // open the shared file
    HANDLE fileHandle = NULL;
    while (fileHandle == NULL) {
        fileHandle = OpenFileMapping(FILE_MAP_READ, FALSE, PPLUSCAMERAMMFNAME);
        if (fileHandle == NULL) {
            fileHandle = CreateFileMapping(
                INVALID_HANDLE_VALUE,
                NULL,
                PAGE_READONLY,
                0,
                m_iImageHeight * m_iImageWidth * 3,
                PPLUSCAMERAMMFNAME
            );
        }
    }
    this->sharedBufferFileHandle = fileHandle;

    sharedBuffer = (unsigned char*)MapViewOfFile(
        fileHandle,
        FILE_MAP_READ,
        0, 0,
        m_iImageHeight * m_iImageWidth * 3
    );

    sharedBufferSemaphore = CreateSemaphore(NULL, 0, 1, PPLUSCAMERASEMAPHORENAME);
}

STDMETHODIMP_(HRESULT __stdcall) PPlusVideoStream::QueryInterface(REFIID riid, void** ppv) {
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

HRESULT __stdcall PPlusVideoStream::SetFormat(AM_MEDIA_TYPE* mediaType) {
    DECLARE_PTR(VIDEOINFOHEADER, pvi, m_mt.pbFormat);
    m_mt = *mediaType;
    IPin* pin;
    ConnectedTo(&pin);
    if (pin) {
        IFilterGraph* pGraph = parentFilter->GetGraph();
        pGraph->Reconnect(this);
    }
    return S_OK;
}

HRESULT __stdcall PPlusVideoStream::GetFormat(AM_MEDIA_TYPE** mediaTypePointer) {
    *mediaTypePointer = CreateMediaType(&m_mt);
    return S_OK;
}

HRESULT __stdcall PPlusVideoStream::GetNumberOfCapabilities(int* countPointer, int* sizePointer) {
    if ((countPointer == NULL)
        || (sizePointer == NULL)) {
        return E_POINTER;
    }
    *countPointer = 1;
    *sizePointer = sizeof(VIDEO_STREAM_CONFIG_CAPS);
    return S_OK;
}

HRESULT __stdcall PPlusVideoStream::GetStreamCaps(int index, AM_MEDIA_TYPE** mediaTypePointer, BYTE* pSCC) {
    if ((mediaTypePointer == NULL)
        || (pSCC == NULL)) {
        return E_POINTER;
    } else if (index < 0) {
        return E_INVALIDARG;
    } else if (index > 1) {
        return S_FALSE;
    }
    
    *mediaTypePointer = CreateMediaType(&m_mt);
    if (*mediaTypePointer == NULL) {
        return E_OUTOFMEMORY;
    }

    DECLARE_PTR(VIDEOINFOHEADER, pvi, (*mediaTypePointer)->pbFormat);

    pvi->bmiHeader.biCompression = BI_RGB;
    pvi->bmiHeader.biBitCount = 24;
    pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth = this->m_iImageWidth;
    pvi->bmiHeader.biHeight = this->m_iImageHeight;
    pvi->bmiHeader.biPlanes = 1;
    pvi->bmiHeader.biSizeImage = GetBitmapSize(&pvi->bmiHeader);
    pvi->bmiHeader.biClrImportant = 0;

    pvi->AvgTimePerFrame = m_rtFrameLength;

    SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

    (*mediaTypePointer)->majortype = MEDIATYPE_Video;
    (*mediaTypePointer)->subtype = MEDIASUBTYPE_RGB24;
    (*mediaTypePointer)->formattype = FORMAT_VideoInfo;
    (*mediaTypePointer)->bTemporalCompression = FALSE;
    (*mediaTypePointer)->bFixedSizeSamples = TRUE;
    (*mediaTypePointer)->lSampleSize = pvi->bmiHeader.biSizeImage;
    (*mediaTypePointer)->cbFormat = sizeof(VIDEOINFOHEADER);

    DECLARE_PTR(VIDEO_STREAM_CONFIG_CAPS, pvscc, pSCC);

    pvscc->guid = FORMAT_VideoInfo;
    pvscc->VideoStandard = AnalogVideo_None;
    pvscc->InputSize.cx = m_iImageWidth;
    pvscc->InputSize.cy = m_iImageHeight;
    pvscc->MinCroppingSize.cx = 320;
    pvscc->MinCroppingSize.cy = 180;
    pvscc->MaxCroppingSize.cx = m_iImageWidth;
    pvscc->MaxCroppingSize.cy = m_iImageHeight;
    pvscc->CropGranularityX = 320;
    pvscc->CropGranularityY = 180;
    pvscc->CropAlignX = 0;
    pvscc->CropAlignY = 0;

    pvscc->MinOutputSize.cx = m_iImageWidth;
    pvscc->MinOutputSize.cy = m_iImageHeight;
    pvscc->MaxOutputSize.cx = m_iImageWidth;
    pvscc->MaxOutputSize.cy = m_iImageHeight;
    pvscc->OutputGranularityX = 0;
    pvscc->OutputGranularityY = 0;
    pvscc->StretchTapsX = 0;
    pvscc->StretchTapsY = 0;
    pvscc->ShrinkTapsX = 0;
    pvscc->ShrinkTapsY = 0;
    pvscc->MinFrameInterval = FPS(5);   //5 fps
    pvscc->MaxFrameInterval = FPS(1); // 0.2 fps
    pvscc->MinBitsPerSecond = (320 * 180 * 3 * 8) / 1;
    pvscc->MaxBitsPerSecond = m_iImageWidth * m_iImageHeight * 3 * 8 * 5;

    return S_OK;
}

// Cannot set any properties
HRESULT __stdcall PPlusVideoStream::Set(REFGUID guidPropSet, DWORD dwID, void* pInstanceData, DWORD cbInstanceData, void* pPropData, DWORD cbPropData) {
    return E_NOTIMPL;
}

// Get: Return the pin category (our only property). 
HRESULT PPlusVideoStream::Get(
    REFGUID guidPropSet,   // Which property set.
    DWORD dwPropID,        // Which property in that set.
    void* pInstanceData,   // Instance data (ignore).
    DWORD cbInstanceData,  // Size of the instance data (ignore).
    void* pPropData,       // Buffer to receive the property data.
    DWORD cbPropData,      // Size of the buffer.
    DWORD* pcbReturned     // Return the size of the property.
) {
    if (guidPropSet != AMPROPSETID_Pin) {
        return E_PROP_SET_UNSUPPORTED;
    }
    if (dwPropID != AMPROPERTY_PIN_CATEGORY) {
        return E_PROP_ID_UNSUPPORTED;
    }
    if (pPropData == NULL && pcbReturned == NULL) {
        return E_POINTER;
    }

    if (pcbReturned) {
        *pcbReturned = sizeof(GUID);
    } 
    if (pPropData == NULL) {
        return S_OK; // Caller just wants to know the size. 
    }
    if (cbPropData < sizeof(GUID)) {
        return E_UNEXPECTED;// The buffer is too small.
    }

    *(GUID*)pPropData = PIN_CATEGORY_CAPTURE;
    return S_OK;
}

HRESULT __stdcall PPlusVideoStream::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD* pTypeSupport) {
    if (guidPropSet != AMPROPSETID_Pin) {
        return E_PROP_SET_UNSUPPORTED;
    }
    if (dwPropID != AMPROPERTY_PIN_CATEGORY) {
        return E_PROP_ID_UNSUPPORTED;
    }
    // We support getting this property, but not setting it.
    if (pTypeSupport) {
        *pTypeSupport = KSPROPERTY_SUPPORT_GET;
    }
    return S_OK;
}

PPlusVideoStream::~PPlusVideoStream() {
    UnmapViewOfFile(sharedBuffer);
    CloseHandle(sharedBufferFileHandle);
}

//
// DecideBufferSize
//
// This will always be called after the format has been sucessfully
// negotiated. So we have a look at m_mt to see what size image we agreed.
// Then we can ask for buffers of the correct size to contain them.
//
HRESULT PPlusVideoStream::DecideBufferSize(IMemAllocator* allocator, ALLOCATOR_PROPERTIES* properties) {
    CheckPointer(allocator, E_POINTER);
    CheckPointer(properties, E_POINTER);

    CAutoLock cAutoLock(m_pFilter->pStateLock());
    HRESULT hr = NOERROR;

    VIDEOINFO* pvi = (VIDEOINFO*)m_mt.Format();
    properties->cBuffers = 1;
    properties->cbBuffer = pvi->bmiHeader.biSizeImage;

    ASSERT(pProperties->cbBuffer);

    // Ask the allocator to reserve us some sample memory. NOTE: the function
    // can succeed (return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted.
    ALLOCATOR_PROPERTIES Actual;
    hr = allocator->SetProperties(properties, &Actual);
    if (FAILED(hr)) {
        return hr;
    }

    // Is this allocator unsuitable?
    if (Actual.cbBuffer < properties->cbBuffer) {
        return E_FAIL;
    }

    // Make sure that we have only 1 buffer (we erase the ball in the
    // old buffer to save having to zero a 200k+ buffer every time
    // we draw a frame)
    ASSERT(Actual.cBuffers == 1);
    return NOERROR;
}

HRESULT PPlusVideoStream::FillBuffer(IMediaSample* pSample) {
    CAutoLock cAutoLockShared(&m_cSharedState);
    if (sharedBuffer == NULL) {
        return S_FALSE;
    }
    
    // Access the sample's data buffer
    BYTE* data;
    long size;
    pSample->GetPointer(&data);
    size = pSample->GetSize();
    
    // Check that we're still using video
    ASSERT(m_mt.formattype == FORMAT_VideoInfo);

    WaitForSingleObject(sharedBufferSemaphore, INFINITE);

    CopyMemory(data, sharedBuffer, size);

    m_pFilter->StreamTime(this->now);
    REFERENCE_TIME startTime = now.m_time + VIDEODELAY;
        //m_iFrameNumber * m_rtFrameLength;
    REFERENCE_TIME stopTime = startTime + m_rtFrameLength;
    
    pSample->SetTime(&startTime, &stopTime);
    pSample->SetSyncPoint(TRUE);

    pSample->SetSyncPoint(TRUE);

    return S_OK;
}

HRESULT PPlusVideoStream::SetMediaType(const CMediaType* pMediaType) {
    CAutoLock cAutoLock(m_pFilter->pStateLock());

    // Pass the call up to my base class
    HRESULT hr = CSourceStream::SetMediaType(pMediaType);

    if (SUCCEEDED(hr)) {
        VIDEOINFO* pvi = (VIDEOINFO*)m_mt.Format();
        if (pvi == NULL) {
            return E_UNEXPECTED;
        }
        
        // Agree only on RGB24
        if (pvi->bmiHeader.biBitCount == 24) {
            this->m_MediaType = *pMediaType;
            //this->m_nCurrentBitDepth = pvi->bmiHeader.biBitCount;
            hr = S_OK;
        } else {
            ASSERT(FALSE);
            hr = E_INVALIDARG;
        }
    }

    return hr;
}

HRESULT PPlusVideoStream::CheckMediaType(const CMediaType* pMediaType) {
    CheckPointer(pMediaType, E_POINTER);

    if ((*(pMediaType->Type()) != MEDIATYPE_Video) ||   // we only output video
        !(pMediaType->IsFixedSize())){                  // in fixed size samples
        return E_INVALIDARG;
    }

    // Check for the subtypes we support
    const GUID* SubType = pMediaType->Subtype();
    if (SubType == NULL) {
        return E_INVALIDARG;
    }
    if (*SubType != MEDIASUBTYPE_RGB24) {
        return E_INVALIDARG;
    }

    // Get the format area of the media type
    VIDEOINFO* pvi = (VIDEOINFO*)pMediaType->Format();

    if (pvi == NULL) {
        return E_INVALIDARG;
    }

    // Check if the image width & height have changed
    if (pvi->bmiHeader.biWidth != m_iImageWidth ||
        abs(pvi->bmiHeader.biHeight) != m_iImageHeight) {
        // If the image width/height is changed, fail CheckMediaType() to force
        // the renderer to resize the image.
        return E_INVALIDARG;
    }
        
    // Don't accept formats with negative height, which would cause the desktop
    // image to be displayed upside down.
    if (pvi->bmiHeader.biHeight < 0) {
        return E_INVALIDARG;
    }

    return S_OK;  // This format is acceptable.
}

HRESULT PPlusVideoStream::GetMediaType(int iPosition, CMediaType* pmt) {
    CheckPointer(pmt, E_POINTER);
    CAutoLock cAutoLock(m_pFilter->pStateLock());

    if (iPosition < 0) {
        return E_INVALIDARG;
    }

    // Only offer one media type
    if (iPosition >= 1) {
        return VFW_S_NO_MORE_ITEMS;
    }

    VIDEOINFO* pvi = (VIDEOINFO*)pmt->AllocFormatBuffer(sizeof(VIDEOINFO));
    if (NULL == pvi) {
        return(E_OUTOFMEMORY);
    }

    ZeroMemory(pvi, sizeof(VIDEOINFO));
    
    // Only supports RGB 24
    pvi->bmiHeader.biCompression =  BI_RGB;
    pvi->bmiHeader.biBitCount = 24;
    pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth = m_iImageWidth;
    pvi->bmiHeader.biHeight = m_iImageHeight;
    pvi->bmiHeader.biPlanes = 1;
    pvi->bmiHeader.biSizeImage = GetBitmapSize(&pvi->bmiHeader);
    pvi->bmiHeader.biClrImportant = 0;
    pvi->AvgTimePerFrame = m_rtFrameLength;

    SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(FALSE);

    // Work out the GUID for the subtype from the header info.
    const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
    pmt->SetSubtype(&SubTypeGUID);
    pmt->SetSampleSize(pvi->bmiHeader.biSizeImage);

    return NOERROR;
}

