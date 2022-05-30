#include "common.h"
#include "PPlusFilter.h"

// Filter setup data
const AMOVIESETUP_MEDIATYPE sudOpPinTypes = {
    &MEDIATYPE_Video,       // Major type
    &MEDIASUBTYPE_NULL      // Minor type
};

const AMOVIESETUP_PIN sudPPlusCameraOutputPin = {
    (LPWSTR)L"Output",      // Obsolete, not used.
    FALSE,          // Is this pin rendered?
    TRUE,           // Is it an output pin?
    FALSE,          // Can the filter create zero instances?
    FALSE,          // Does the filter create multiple instances?
    &CLSID_NULL,    // Obsolete.
    NULL,           // Obsolete.
    1,              // Number of media types.
    &sudOpPinTypes  // Pointer to media types.
};

const AMOVIESETUP_FILTER sudPPlusCamera = {
    &CLSID_PPlusCamera,// Filter CLSID
    PPLUSCAMERANAME,       // String name
    MERIT_DO_NOT_USE,       // Filter merit
    1,                      // Number pins
    &sudPPlusCameraOutputPin    // Pin details
};

CFactoryTemplate g_Templates[] = {
    {
        PPLUSCAMERANAME,            // Name
        &CLSID_PPlusCamera,         // clsid
        PPlusVideo::CreateInstance, // Method to create an instance
        NULL,                       // Initialization function
        &sudPPlusCamera             // Set-up information
    }
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

const REGFILTER2 PPlusCameraReg = {
    2,
    MERIT_NORMAL,
    1,
    &sudPPlusCameraOutputPin
};

STDAPI DllRegisterServer() {
    HRESULT result;
    IFilterMapper2* filterMapper = NULL;

    result = AMovieDllRegisterServer2(TRUE);
    if (FAILED(result)) {
        return result;
    }

    result = CoCreateInstance(
        CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
        IID_IFilterMapper2, (void**)&filterMapper
    );
    if (FAILED(result)) {
        return result;
    }

    IMoniker* moniker = NULL;
    result = filterMapper->RegisterFilter(
        CLSID_PPlusCamera,
        PPLUSCAMERANAME,
        &moniker,
        &CLSID_VideoInputDeviceCategory,
        NULL,
        &PPlusCameraReg
    );
    filterMapper->Release();
    return result;
}

STDAPI DllUnregisterServer() {
    HRESULT result;
    result = AMovieDllRegisterServer2(FALSE);
    if (FAILED(result)) {
        return result;
    }

    IFilterMapper2* filterMapper = NULL;
    result = CoCreateInstance(
        CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
        IID_IFilterMapper2, (void**)&filterMapper
    );
    if (FAILED(result)) {
        return result;
    }

    result = filterMapper->UnregisterFilter(
        &CLSID_VideoInputDeviceCategory,
        NULL,
        CLSID_PPlusCamera
    );
    filterMapper->Release();
    return result;
}

//
// DllEntryPoint
//
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule,
    DWORD  dwReason,
    LPVOID lpReserved) {
    return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}