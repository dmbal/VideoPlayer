#pragma once
#include "Common.h"
#include <Mfidl.h>
#include <mfapi.h>
#include "mftransform.h"
#include <MMSystem.h>

// {88ACF5E6-2ED1-4780-87B1-D71814C2D42A}
DEFINE_GUID(IID_SampleTransform, 0x33acf5e6, 0x2ed1, 0x4780, 0x87, 0xb1, 0xd7, 0x18, 0x14, 0xc2,
    0xd4, 0x2a);
// Microsoft-specific extension necessary to support the __uuidof(IAsyncState) notation.
class __declspec(uuid("33ACF5E6-2ED1-4780-87B1-D71814C2D42A")) SampleTransform;

class SampleTransform :
    public IMFTransform
{
public:
    SampleTransform(bool startTimestampsFromZero);
    ~SampleTransform(void);

    //
    // IMFTransform stream handling functions
    STDMETHODIMP GetStreamLimits(DWORD* pdwInputMinimum, DWORD* pdwInputMaximum,
        DWORD* pdwOutputMinimum, DWORD* pdwOutputMaximum);

    STDMETHODIMP GetStreamIDs(DWORD dwInputIDArraySize, DWORD* pdwInputIDs,
        DWORD dwOutputIDArraySize, DWORD* pdwOutputIDs);

    STDMETHODIMP GetStreamCount(DWORD* pcInputStreams, DWORD* pcOutputStreams);
    STDMETHODIMP GetInputStreamInfo(DWORD dwInputStreamID,
        MFT_INPUT_STREAM_INFO* pStreamInfo);
    STDMETHODIMP GetOutputStreamInfo(DWORD dwOutputStreamID,
        MFT_OUTPUT_STREAM_INFO* pStreamInfo);
    STDMETHODIMP GetInputStreamAttributes(DWORD dwInputStreamID,
        IMFAttributes** pAttributes);
    STDMETHODIMP GetOutputStreamAttributes(DWORD dwOutputStreamID,
        IMFAttributes** pAttributes);
    STDMETHODIMP DeleteInputStream(DWORD dwStreamID);
    STDMETHODIMP AddInputStreams(DWORD cStreams, DWORD* adwStreamIDs);

    //
    // IMFTransform mediatype handling functions
    STDMETHODIMP GetInputAvailableType(DWORD dwInputStreamID, DWORD dwTypeIndex,
        IMFMediaType** ppType);
    STDMETHODIMP GetOutputAvailableType(DWORD dwOutputStreamID, DWORD dwTypeIndex,
        IMFMediaType** ppType);
    STDMETHODIMP SetInputType(DWORD dwInputStreamID, IMFMediaType* pType,
        DWORD dwFlags);
    STDMETHODIMP SetOutputType(DWORD dwOutputStreamID, IMFMediaType* pType,
        DWORD dwFlags);
    STDMETHODIMP GetInputCurrentType(DWORD dwInputStreamID, IMFMediaType** ppType);
    STDMETHODIMP GetOutputCurrentType(DWORD dwOutputStreamID, IMFMediaType** ppType);

    //
    // IMFTransform status and eventing functions
    STDMETHODIMP GetInputStatus(DWORD dwInputStreamID, DWORD* pdwFlags);
    STDMETHODIMP GetOutputStatus(DWORD* pdwFlags);
    STDMETHODIMP SetOutputBounds(LONGLONG hnsLowerBound, LONGLONG hnsUpperBound);
    STDMETHODIMP ProcessEvent(DWORD dwInputStreamID, IMFMediaEvent* pEvent);
    STDMETHODIMP GetAttributes(IMFAttributes** pAttributes);


    //
    // IMFTransform main data processing and command functions
    STDMETHODIMP ProcessMessage(MFT_MESSAGE_TYPE eMessage, ULONG_PTR ulParam);
    STDMETHODIMP ProcessInput(DWORD dwInputStreamID, IMFSample* pSample,
        DWORD dwFlags);

    STDMETHODIMP ProcessOutput(DWORD dwFlags, DWORD cOutputBufferCount,
        MFT_OUTPUT_DATA_BUFFER* pOutputSamples, DWORD* pdwStatus);

    //
    // IUnknown interface implementation
    //
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);


private:

    MFTIME prevBefore, prevAfter;
    volatile long m_cRef;                             // ref count
    CComAutoCriticalSection m_critSec;       // critical section for the MFT
    MFTIME m_timeOffset = 0;
    bool m_startTimestampsFromZero;
    LONGLONG m_FirstSampleTimestamp;
    LONGLONG m_PrevTimestamp;
    bool m_FirstSampleTimestampInitialized;
    CComPtr<IMFSample>  m_pSample;           // Input sample.
    CComPtr<IMFMediaType> m_pInputType;      // Input media type.
    CComPtr<IMFMediaType> m_pOutputType;     // Output media type.
                                             // private helper functions
    HRESULT GetSupportedMediaType(DWORD dwTypeIndex, IMFMediaType** ppmt);
    HRESULT CheckMediaType(IMFMediaType *pmt);
};