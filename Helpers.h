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
HRESULT CopyType(IMFMediaType * in_media_type, IMFMediaType * out_mf_media_type);
HRESULT CopyAudioType(IMFMediaType * in_media_type, IMFMediaType * out_mf_media_type);
HRESULT CopyVideoType(IMFMediaType * in_media_type, IMFMediaType * out_mf_media_type);
IMFMediaType * CreateMediaType(GUID major, GUID minor);
