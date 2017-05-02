#pragma once
#include "Common.h"
#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <mfapi.h>

IMFMediaType* GetMediaType(IMFStreamDescriptor * pStreamDescriptor);
GUID GetSubtype(IMFMediaType * mediaType);
GUID GetMajorType(IMFMediaType * mediaType);
HRESULT CopyAttribute(IMFAttributes *pSrc, IMFAttributes *pDest, const GUID& key);
IMFMediaType * CreateMediaType(GUID major, GUID minor);
IMFTransform* FindEncoderTransform(GUID major, GUID minor);
