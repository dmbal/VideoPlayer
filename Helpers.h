#pragma once
#include "Common.h"
#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <mftransform.h>
#include <mfapi.h>
#include <sstream>
#include <string>

IMFMediaType* GetMediaType(IMFStreamDescriptor * pStreamDescriptor);
GUID GetSubtype(IMFMediaType * mediaType);
GUID GetMajorType(IMFMediaType * mediaType);
HRESULT CopyAttribute(IMFAttributes *pSrc, IMFAttributes *pDest, const GUID& key);
IMFMediaType * CreateMediaType(GUID major, GUID minor);
IMFTransform* FindEncoderTransform(GUID major, GUID minor);

HRESULT UnwrapPartialTopo(IMFTopologyNode * node, int level);
HRESULT CopyVideoType(IMFMediaType * in_media_type, IMFMediaType * out_mf_media_type);
