#pragma once
#include <streams.h>
#include <initguid.h>

// Filter name strings
#define PPLUSCAMERANAME			L"PPlus Camera"
#define PPLUSCAMERAMMFNAME		L"PPlusCameraSharedBuffer"
#define PPLUSCAMERASEMAPHORENAME	L"Global\\PPlusVideoFrameSemaphore"

#define PPLUSAUDIONAME			L"PPlus Audio"
#define PPLUSAUDIOMMFNAME		L"PPlusAudioSharedBuffer"
#define PPLUSAUDIOSEMAPHORENAME		L"Global\\PPlusAudioSemaphore"

#define DECLARE_PTR(type, ptr, expr) type* ptr = (type*)(expr);

#define FPS(x)				(UNITS / x)
#define VIDEODELAY			(UNITS/10)
#define VIDEOHEIGHT			1080
#define VIDEOWIDTH			1920