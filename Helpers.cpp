#include "Helpers.h"

GUID GetSubtype(IMFMediaType * mediaType) {
    GUID minorType;
    HRESULT hr = mediaType->GetGUID(MF_MT_SUBTYPE, &minorType);
    return minorType;
}

GUID GetMajorType(IMFMediaType * mediaType) {
    GUID major;
    HRESULT hr = mediaType->GetMajorType(&major);
    return major;
}

HRESULT CopyAttribute(IMFAttributes *pSrc, IMFAttributes *pDest, const GUID& key)
{
    PROPVARIANT var;
    PropVariantInit(&var);
    HRESULT hr = S_OK;

    hr = pSrc->GetItem(key, &var);
    if (SUCCEEDED(hr))
    {
        hr = pDest->SetItem(key, var);
    }
    PropVariantClear(&var);
    return hr;
}

HRESULT CopyVideoType(IMFMediaType * in_media_type, IMFMediaType * out_mf_media_type) {
    UINT32 frameRate = 0;
    UINT32 frameRateDenominator;
    UINT32 aspectRatio = 0;
    UINT32 denominator = 0;
    UINT32 width, height, bitrate;
    HRESULT hr = S_OK;
    if (SUCCEEDED(in_media_type->GetUINT32(MF_MT_AVG_BITRATE, &bitrate)))
    {
        out_mf_media_type->SetUINT32(MF_MT_AVG_BITRATE, bitrate);
    }
    hr = MFGetAttributeRatio(in_media_type, MF_MT_FRAME_SIZE, &width, &height);
    hr = MFGetAttributeRatio(in_media_type, MF_MT_FRAME_RATE, &frameRate, &frameRateDenominator);
    hr = MFGetAttributeRatio(in_media_type, MF_MT_PIXEL_ASPECT_RATIO, &aspectRatio, &denominator);
    hr = MFSetAttributeRatio(out_mf_media_type, MF_MT_FRAME_SIZE, width, height);
    hr = MFSetAttributeRatio(out_mf_media_type, MF_MT_FRAME_RATE, frameRate, frameRateDenominator);
    hr = MFSetAttributeRatio(out_mf_media_type, MF_MT_PIXEL_ASPECT_RATIO, aspectRatio, denominator);
    hr = CopyAttribute(in_media_type, out_mf_media_type, MF_MT_INTERLACE_MODE);
    return hr;
}

HRESULT CopyAudioType(IMFMediaType * in_media_type, IMFMediaType * out_mf_media_type) {
    HRESULT hr = S_OK;
    hr = CopyAttribute(in_media_type, out_mf_media_type, MF_MT_AUDIO_NUM_CHANNELS);
    hr = CopyAttribute(in_media_type, out_mf_media_type, MF_MT_AUDIO_SAMPLES_PER_SECOND);
    hr = CopyAttribute(in_media_type, out_mf_media_type, MF_MT_AUDIO_BLOCK_ALIGNMENT);
    hr = CopyAttribute(in_media_type, out_mf_media_type, MF_MT_AUDIO_AVG_BYTES_PER_SECOND);
    hr = CopyAttribute(in_media_type, out_mf_media_type, MF_MT_AVG_BITRATE);
    return hr;
}

HRESULT CopyType(IMFMediaType * in_media_type, IMFMediaType * out_mf_media_type) {
    GUID major = GetMajorType(in_media_type);
    HRESULT hr = S_OK;

    if (major == MFMediaType_Audio)
    {
        hr = CopyAudioType(in_media_type, out_mf_media_type);
    }
    else if (major == MFMediaType_Video)
    {
        hr = CopyVideoType(in_media_type, out_mf_media_type);
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}