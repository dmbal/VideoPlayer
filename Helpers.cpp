#include "Helpers.h"
#include <array>
#include <map>
#include "SampleTransform.h"

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

HRESULT CopyVideoType(IMFMediaType * sourceMediaType, IMFMediaType * destMediaType) {
    UINT32 frameRate = 0;
    UINT32 frameRateDenominator;
    UINT32 aspectRatio = 0;
    UINT32 denominator = 0;
    UINT32 width, height, bitrate;
    HRESULT hr = S_OK;

    //in_media_type->CopyAllItems(out_mf_media_type);

    if (SUCCEEDED(sourceMediaType->GetUINT32(MF_MT_AVG_BITRATE, &bitrate)))
    {
        destMediaType->SetUINT32(MF_MT_AVG_BITRATE, bitrate);
    }
    hr = MFGetAttributeRatio(sourceMediaType, MF_MT_FRAME_SIZE, &width, &height);
    THROW_ON_FAIL(hr);
    hr = MFGetAttributeRatio(sourceMediaType, MF_MT_FRAME_RATE, &frameRate, &frameRateDenominator);
    THROW_ON_FAIL(hr);
    hr = MFGetAttributeRatio(sourceMediaType, MF_MT_PIXEL_ASPECT_RATIO, &aspectRatio, &denominator);
    THROW_ON_FAIL(hr);
    hr = MFSetAttributeRatio(destMediaType, MF_MT_FRAME_SIZE, width, height);
    THROW_ON_FAIL(hr);
    hr = MFSetAttributeRatio(destMediaType, MF_MT_FRAME_RATE, frameRate, frameRateDenominator);
    THROW_ON_FAIL(hr);
    hr = MFSetAttributeRatio(destMediaType, MF_MT_PIXEL_ASPECT_RATIO, aspectRatio, denominator);
    THROW_ON_FAIL(hr);
    hr = CopyAttribute(sourceMediaType, destMediaType, MF_MT_INTERLACE_MODE);
    THROW_ON_FAIL(hr);
    return hr;
}

HRESULT CopyAudioType(IMFMediaType * sourceMediaType, IMFMediaType * destMediaType) {
    HRESULT hr = S_OK;
    hr = CopyAttribute(sourceMediaType, destMediaType, MF_MT_AUDIO_NUM_CHANNELS);
    THROW_ON_FAIL(hr);
    hr = CopyAttribute(sourceMediaType, destMediaType, MF_MT_AUDIO_SAMPLES_PER_SECOND);
    THROW_ON_FAIL(hr);
    hr = CopyAttribute(sourceMediaType, destMediaType, MF_MT_AUDIO_BLOCK_ALIGNMENT);
    THROW_ON_FAIL(hr);
    hr = CopyAttribute(sourceMediaType, destMediaType, MF_MT_AUDIO_AVG_BYTES_PER_SECOND);
    THROW_ON_FAIL(hr);
    hr = CopyAttribute(sourceMediaType, destMediaType, MF_MT_AVG_BITRATE);
    THROW_ON_FAIL(hr);
    return hr;
}

HRESULT CopyType(IMFMediaType * sourceMediaType, IMFMediaType * destMediaType) {
    GUID major = GetMajorType(sourceMediaType);
    HRESULT hr = S_OK;

    if (major == MFMediaType_Audio)
    {
        hr = CopyAudioType(sourceMediaType, destMediaType);
    }
    else if (major == MFMediaType_Video)
    {
        hr = CopyVideoType(sourceMediaType, destMediaType);
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


/*****************************DELETE EVERYTHING BELOW************************/
inline bool operator< (const GUID &firstGUID, const GUID &secondGUID) {
    return (memcmp(&firstGUID, &secondGUID, sizeof(GUID)) < 0 ? true : false);
}

const std::map<GUID, std::wstring> video_type_map = {
    { MFVideoFormat_RGB8, L"MFVideoFormat_RGB8" },
    { MFVideoFormat_RGB555, L"MFVideoFormat_RGB555" },
    { MFVideoFormat_RGB565, L"MFVideoFormat_RGB565" },
    { MFVideoFormat_RGB24, L"MFVideoFormat_RGB24" },
    { MFVideoFormat_RGB32, L"MFVideoFormat_RGB32" },
    { MFVideoFormat_ARGB32, L"MFVideoFormat_ARGB32" },
    { MFVideoFormat_AI44,  L"MFVideoFormat_AI44" },
    { MFVideoFormat_AYUV,  L"MFVideoFormat_AYUV" },
    { MFVideoFormat_I420,  L"MFVideoFormat_I420" },
    { MFVideoFormat_IYUV,  L"MFVideoFormat_IYUV" },
    { MFVideoFormat_NV11,  L"MFVideoFormat_NV11" },
    { MFVideoFormat_NV12,  L"MFVideoFormat_NV12" },
    { MFVideoFormat_UYVY,  L"MFVideoFormat_UYVY" },
    { MFVideoFormat_Y41P,  L"MFVideoFormat_Y41P" },
    { MFVideoFormat_Y41T,  L"MFVideoFormat_Y41T" },
    { MFVideoFormat_Y42T,  L"MFVideoFormat_Y42T" },
    { MFVideoFormat_YUY2,  L"MFVideoFormat_YUY2" },
    { MFVideoFormat_YVU9,  L"MFVideoFormat_YVU9" },
    { MFVideoFormat_YV12,  L"MFVideoFormat_YV12" },
    { MFVideoFormat_YVYU,  L"MFVideoFormat_YVYU" },
    { MFVideoFormat_P010,  L"MFVideoFormat_P010" },
    { MFVideoFormat_P016,  L"MFVideoFormat_P016" },
    { MFVideoFormat_P210,  L"MFVideoFormat_P210" },
    { MFVideoFormat_P216,  L"MFVideoFormat_P216" },
    { MFVideoFormat_v210,  L"MFVideoFormat_v210" },
    { MFVideoFormat_v216,  L"MFVideoFormat_v216" },
    { MFVideoFormat_v410,  L"MFVideoFormat_v410" },
    { MFVideoFormat_Y210,  L"MFVideoFormat_Y210" },
    { MFVideoFormat_Y216,  L"MFVideoFormat_Y216" },
    { MFVideoFormat_Y410,  L"MFVideoFormat_Y410" },
    { MFVideoFormat_Y416,  L"MFVideoFormat_Y416" },
    { MFVideoFormat_DV25,  L"MFVideoFormat_DV25" },
    { MFVideoFormat_DV50,  L"MFVideoFormat_DV50" },
    { MFVideoFormat_DVC,   L"MFVideoFormat_DVC" },
    { MFVideoFormat_H264,  L"MFVideoFormat_H264" },
    { MFVideoFormat_H263, L"MFVideoFormat_H263" },
    { MFVideoFormat_DVSL, L"MFVideoFormat_DVSL" },
    { MFVideoFormat_H264_ES, L"MFVideoFormat_H264_ES" },
    { MFVideoFormat_MJPG, L"MFVideoFormat_MJPG" },
    { MFVideoFormat_WMV1, L"MFVideoFormat_WMV1" },
    { MFVideoFormat_WMV2, L"MFVideoFormat_WMV2" },
    { MFVideoFormat_WMV3, L"MFVideoFormat_WMV3" },
};
const std::map<GUID, std::wstring> major_type_map = {
    { MFMediaType_Audio, L"MFMediaType_Audio" },
    { MFMediaType_Binary, L"MFMediaType_Binary" },
    { MFMediaType_FileTransfer, L"MFMediaType_FileTransfer" },
    { MFMediaType_HTML, L"MFMediaType_HTML" },
    { MFMediaType_Image, L"MFMediaType_Image" },
    { MFMediaType_Protected, L"MFMediaType_Protected" },
    { MFMediaType_SAMI,  L"MFMediaType_SAMI" },
    { MFMediaType_Script,  L"MFMediaType_Script" },
    { MFMediaType_Stream,  L"MFMediaType_Stream" },
    { MFMediaType_Video,  L"MFMediaType_Video" }
};

std::wstring guidToString(GUID guid) {
    std::array<wchar_t, 40> output;
    wnsprintf(output.data(), output.size(), L"{%08X-%04hX-%04hX-%02X%02X-%02X%02X%02X%02X%02X%02X}", guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    return std::wstring(output.data());
}

std::wstring DetectSubtype(GUID guid) {
    std::wstring  str;
    if (video_type_map.count(guid) != 0) {
        str = video_type_map.find(guid)->second;
        return str;
    }
    else {
        return L"undefined subtype " + guidToString(guid);
    }
}

std::wstring DetectMajorType(GUID guid) {
    std::wstring  str;
    if (major_type_map.count(guid) != 0) {
        str = major_type_map.find(guid)->second;
        return str.c_str();
    }
    else {
        return L"undefined major type " + guidToString(guid);
    }

}

void DebugInfo(std::wstring info) {
    std::wstring strng;
    std::wstringstream strstream;
    strstream << info;
    strng = strstream.str();
    strstream.str(L"");
    strstream.clear();
    OutputDebugStringW(strng.c_str());
}

void DebugInfoWithLevel(std::wstring pref, int level) {

    DebugInfo(L"\\");
    for (int i = 0; i < level; i++)
    {
        DebugInfo(L"--");
    }
    DebugInfo(pref);
}

HRESULT HandleChildren(IMFTopologyNode * node, int level) {
    DWORD outputs = 0;
    HRESULT hr = node->GetOutputCount(&outputs);
    for (int i = 0; i < outputs; i++) {
        DWORD nextInput = 0;
        CComPtr<IMFTopologyNode> childNode;
        hr = node->GetOutput(0, &childNode, &nextInput);
        if (SUCCEEDED(hr))
        {
            UnwrapPartialTopo(childNode, level);
        }
    }
    return hr;
}

HRESULT HandleNodeObject(IMFTopologyNode * node) {

    CComPtr<IUnknown> unknown;
    HRESULT hr = node->GetObjectW(&unknown);
    if (SUCCEEDED(hr)) {
        DebugInfo(L"(");
        CComPtr<IMFActivate> activate;
        unknown->QueryInterface(IID_PPV_ARGS(&activate));
        if (activate != NULL) {
            GUID clsId;
            activate->GetGUID(MF_TOPONODE_TRANSFORM_OBJECTID, &clsId);
            DebugInfo(L"activate");
        }

        CComPtr<IMFTransform> transform;
        unknown->QueryInterface(IID_PPV_ARGS(&transform));
        if (transform != NULL) {
            CComPtr<SampleTransform> sampleTrasform;
            unknown->QueryInterface(IID_PPV_ARGS(&sampleTrasform));
            if (sampleTrasform != NULL)
            {
                DebugInfo(L"(type is SampleTransform) ");
            }
            CComPtr<IMFMediaType> inputType;
            hr = transform->GetInputCurrentType(0, &inputType);
            if (SUCCEEDED(hr)) {
                DebugInfo(L"input type: ");
                DebugInfo(DetectMajorType(GetMajorType(inputType)));
                DebugInfo(L" ");
                DebugInfo(DetectSubtype(GetSubtype(inputType)));
                DebugInfo(L"; ");
            }
            CComPtr<IMFMediaType> outputType;
            hr = transform->GetOutputCurrentType(0, &outputType);
            if (SUCCEEDED(hr)) {
                DebugInfo(L"output type: ");
                DebugInfo(DetectMajorType(GetMajorType(outputType)));
                DebugInfo(L" ");
                DebugInfo(DetectSubtype(GetSubtype(outputType)));
            }

        }

        CComPtr<IMFStreamSink> streamSink;
        unknown->QueryInterface(IID_PPV_ARGS(&streamSink));
        if (streamSink != NULL) {
            CComPtr<IMFMediaTypeHandler> mediaTypeHandler;
            hr = streamSink->GetMediaTypeHandler(&mediaTypeHandler);
            CComPtr<IMFMediaType> inputType;
            mediaTypeHandler->GetCurrentMediaType(&inputType);
            if (SUCCEEDED(hr)) {
                DebugInfo(L"input type: ");
                DebugInfo(DetectMajorType(GetMajorType(inputType)));
                DebugInfo(DetectSubtype(GetSubtype(inputType)));

            }
        }

        DebugInfo(L")");
    }
    return hr;
}

HRESULT HandleSourceNode(IMFTopologyNode * node) {
    DebugInfoWithLevel(L"[SOURCE]", 0);
    HandleNodeObject(node);
    DebugInfo(L"\n");
    HRESULT hr = HandleChildren(node, 1);
    return hr;
}


HRESULT HandleTeeNode(IMFTopologyNode * node, int level) {
    DebugInfoWithLevel(L"[TEE]", level);
    HandleNodeObject(node);
    DebugInfo(L"\n");
    HRESULT hr = HandleChildren(node, level + 1);
    return hr;
}

HRESULT HandleTransformNode(IMFTopologyNode * node, int level) {
    DebugInfoWithLevel(L"[TRANSFORM]", level);
    HandleNodeObject(node);
    DebugInfo(L"\n");
    HRESULT hr = HandleChildren(node, level + 1);
    return hr;
}

HRESULT HandleSinkNode(IMFTopologyNode * node, int level) {
    DebugInfoWithLevel(L"[SINK]", level);
    HandleNodeObject(node);
    DebugInfo(L"\n");
    HRESULT hr = HandleChildren(node, level + 1);
    return hr;
}

HRESULT UnwrapPartialTopo(IMFTopologyNode * node, int level) {
    MF_TOPOLOGY_TYPE type;
    HRESULT hr = node->GetNodeType(&type);

    if (type == MF_TOPOLOGY_SOURCESTREAM_NODE) {
    HandleSourceNode(node);
    }
    if (type == MF_TOPOLOGY_TEE_NODE) {
    HandleTeeNode(node, level);
    }
    if (type == MF_TOPOLOGY_OUTPUT_NODE) {
    HandleSinkNode(node, level);
    }
    if (type == MF_TOPOLOGY_TRANSFORM_NODE) {
    HandleTransformNode(node, level);
    }
    return hr;
}