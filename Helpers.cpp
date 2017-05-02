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

IMFMediaType * CreateMediaType(GUID major, GUID minor) 
{
    CComPtr<IMFMediaType> outputType = NULL;
    do
    {
        HRESULT hr = MFCreateMediaType(&outputType);
        BREAK_ON_FAIL(hr);
        hr = outputType->SetGUID(MF_MT_MAJOR_TYPE, major);
        BREAK_ON_FAIL(hr);
        hr = outputType->SetGUID(MF_MT_SUBTYPE, minor);
        BREAK_ON_FAIL(hr);
    } while (false);

    return outputType.Detach();
}


IMFTransform* FindEncoderTransform(GUID major, GUID minor) {
    UINT32 count = 0;
    IMFActivate **ppActivate = NULL;
    MFT_REGISTER_TYPE_INFO info = { 0 };
    info.guidMajorType = major;
    info.guidSubtype = minor;

    MFTEnumEx(
        MFT_CATEGORY_VIDEO_ENCODER,
        0x00000073,
        NULL,       // Input type
        &info,      // Output type
        &ppActivate,
        &count
    );

    if (count == 0)
    {
        THROW_ON_FAIL(MF_E_TOPO_CODEC_NOT_FOUND);
    }
    HRESULT hr;
    IMFTransform *pEncoder;
    // Create the first encoder in the list.
    THROW_ON_FAIL(ppActivate[0]->ActivateObject(__uuidof(IMFTransform), (void**)&pEncoder));

    CoTaskMemFree(ppActivate);
    return pEncoder;
}