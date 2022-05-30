#pragma once
#include <streams.h>
#include <initguid.h>

// Filter name strings
#define PPLUSCAMERANAME			L"PPlus Camera"
#define PPLUSCAMERAMMFNAME		L"PPlusCameraSharedBuffer"
#define DECLARE_PTR(type, ptr, expr) type* ptr = (type*)(expr);

#define FPS(x)				(UNITS / x)
#define VIDEOHEIGHT			1080
#define VIDEOWIDTH			1920