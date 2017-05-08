#pragma once
#include "SampleTransform.h"
#include <mfapi.h>
SampleTransform::SampleTransform(bool startTimestampsFromZero) :
    m_cRef(1),
    m_startTimestampsFromZero(startTimestampsFromZero),
    m_FirstSampleTimestamp(0),
    m_FirstSampleTimestampInitialized(false),
    m_PrevTimestamp(0),
    prevAfter(0)
{
}


SampleTransform::~SampleTransform(void)
{
    // reduce the count of DLL handles so that we can unload the DLL when 
    // components in it are no longer being used
}



//
// IUnknown interface implementation
//
ULONG SampleTransform::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG SampleTransform::Release()
{
    ULONG refCount = InterlockedDecrement(&m_cRef);
    if (refCount == 0)
    {
        delete this;
    }

    return refCount;
}

HRESULT SampleTransform::QueryInterface(REFIID riid, void** ppv)
{
    HRESULT hr = S_OK;

    if (ppv == NULL)
    {
        return E_POINTER;
    }

    if (riid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this);
    }
    else if (riid == IID_IMFTransform)
    {
        *ppv = static_cast<IMFTransform*>(this);
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }

    if (SUCCEEDED(hr))
        AddRef();

    return hr;
}






//*************************************************************************************
//
// IMFTransform interface implementation
//
//*************************************************************************************


//
// Get the maximum and minimum number of streams that this MFT supports.
//
HRESULT SampleTransform::GetStreamLimits(
    DWORD   *pdwInputMinimum,
    DWORD   *pdwInputMaximum,
    DWORD   *pdwOutputMinimum,
    DWORD   *pdwOutputMaximum)
{
    if (pdwInputMinimum == NULL ||
        pdwInputMaximum == NULL ||
        pdwOutputMinimum == NULL ||
        pdwOutputMaximum == NULL)
    {
        return E_POINTER;
    }

    // This MFT supports only one input stream and one output stream.
    // There can't be more or less than one input or output streams.
    *pdwInputMinimum = 1;
    *pdwInputMaximum = 1;
    *pdwOutputMinimum = 1;
    *pdwOutputMaximum = 1;

    return S_OK;
}


//
// Get the actual number of streams that the MFT is currently set up to
// process.  This is needed in cases the number of streams that an MFT is 
// processing can change depending on various conditions.
//
HRESULT SampleTransform::GetStreamCount(
    DWORD   *pcInputStreams,
    DWORD   *pcOutputStreams)
{
    // check the pointers
    if (pcInputStreams == NULL || pcOutputStreams == NULL)
    {
        return E_POINTER;
    }

    // The MFT supports only one input stream and one output stream.
    *pcInputStreams = 1;
    *pcOutputStreams = 1;

    return S_OK;
}



//
// Get IDs for the input and output streams. This function doesn't need to be implemented in
// this case because the MFT supports only a single input stream and a single output stream, 
// and we can set its ID to 0.
//
HRESULT SampleTransform::GetStreamIDs(
    DWORD   dwInputIDArraySize,
    DWORD   *pdwInputIDs,
    DWORD   dwOutputIDArraySize,
    DWORD   *pdwOutputIDs)
{
    return E_NOTIMPL;
}


//
// Get a structure with information about an input stream with the specified index.
//
HRESULT SampleTransform::GetInputStreamInfo(
    DWORD                   dwInputStreamID,  // stream being queried.
    MFT_INPUT_STREAM_INFO*  pStreamInfo)      // stream information
{
    HRESULT hr = S_OK;

    do
    {
        // lock the MFT - the lock will disengage when variable goes out of scope
        CComCritSecLock<CComAutoCriticalSection> lock(m_critSec);

        BREAK_ON_NULL(pStreamInfo, E_POINTER);

        // This MFT supports only stream with ID of zero
        if (dwInputStreamID != 0)
        {
            hr = MF_E_INVALIDSTREAMNUMBER;
            break;
        }

        // The dwFlags variable contains the required configuration of the input stream. The
        // flags specified here indicate:
        //   - MFT accepts samples with whole units of data.  In this case this means that 
        //      each sample should contain a whole uncompressed frame.
        //   - The samples returned will have only a single buffer.
        pStreamInfo->dwFlags = MFT_INPUT_STREAM_WHOLE_SAMPLES |
            MFT_INPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER;

        // maximum amount of input data that the MFT requires to start returning samples
        pStreamInfo->cbMaxLookahead = 0;

        // memory alignment of the sample buffers
        pStreamInfo->cbAlignment = 0;

        // maximum latency between an input sample arriving and the output sample being
        // ready
        pStreamInfo->hnsMaxLatency = 0;

        // required input size of a sample - 0 indicates that any size is acceptable
        pStreamInfo->cbSize = 0;
    } while (false);

    return hr;
}





//
// Get information about the specified output stream.  Note that the returned structure 
// contains information independent of the media type set on the MFT, and thus should 
// always return values indicating its internal behavior.
//
HRESULT SampleTransform::GetOutputStreamInfo(
    DWORD                     dwOutputStreamID,
    MFT_OUTPUT_STREAM_INFO *  pStreamInfo)
{
    HRESULT hr = S_OK;

    do
    {
        // lock the MFT - the lock will disengage when variable goes out of scope
        CComCritSecLock<CComAutoCriticalSection> lock(m_critSec);

        BREAK_ON_NULL(pStreamInfo, E_POINTER);

        // The MFT supports only a single stream with ID of 0
        if (dwOutputStreamID != 0)
        {
            hr = MF_E_INVALIDSTREAMNUMBER;
            break;
        }

        // The dwFlags variable contains a set of flags indicating how the MFT behaves.
        // The flags shown below indicate the following:
        //   - MFT provides samples with whole units of data.  This means that each sample 
        //     contains a whole uncompressed frame.
        //   - The samples returned will have only a single buffer.
        //   - All of the samples produced by the MFT will have a fixed size.
        //   - The MFT provides samples and there is no need to give it output samples to 
        //     fill in during its ProcessOutput() calls.
        pStreamInfo->dwFlags =
            MFT_OUTPUT_STREAM_WHOLE_SAMPLES |
            MFT_OUTPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER |
            MFT_OUTPUT_STREAM_FIXED_SAMPLE_SIZE | MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES |
            MFT_OUTPUT_STREAM_PROVIDES_SAMPLES;

        // the cbAlignment variable contains information about byte alignment of the sample 
        // buffers, if one is needed.  Zero indicates that no specific alignment is needed.
        pStreamInfo->cbAlignment = 0;

        // Size of the samples returned by the MFT.  Since the MFT provides its own samples,
        // this value must be zero.
        pStreamInfo->cbSize = 0;
    } while (false);

    return hr;
}




//
// Get the bag of custom attributes associated with this MFT.  If the MFT does not support
// any custom attributes, the method can be left unimplemented.  If an object is returned,
// the object can be used to either get or set attributes of this MFT, and thus provide custom
// parameters and information about the MFT.
//
HRESULT SampleTransform::GetAttributes(IMFAttributes** pAttributes)
{
    // This MFT does not support any attributes, so the method is not implemented.
    return E_NOTIMPL;
}



//
// Gets the store of attributes associated with a specified input stream of the 
// MFT.  This method can be left unimplemented if custom input stream attributes 
// are not supported.
//
HRESULT SampleTransform::GetInputStreamAttributes(
    DWORD           dwInputStreamID,
    IMFAttributes** ppAttributes)
{
    // This MFT does not support any attributes, so the method is not implemented.
    return E_NOTIMPL;
}



//
// Gets the store of attributes associated with a specified output stream of the 
// MFT.  This method can be left unimplemented if custom output stream attributes 
// are not supported.
//
HRESULT SampleTransform::GetOutputStreamAttributes(
    DWORD           dwOutputStreamID,
    IMFAttributes** ppAttributes)
{
    // This MFT does not support any attributes, so the method is not implemented.
    return E_NOTIMPL;
}



//
// Deletes the specified input stream from the MFT.  This MFT has only a single 
// constant stream, and therefore this method is not implemented.
//
HRESULT SampleTransform::DeleteInputStream(DWORD dwStreamID)
{
    return E_NOTIMPL;
}



//
// Add the specified input stream from the MFT.  This MFT has only a single 
// constant stream, and therefore this method is not implemented.
//
HRESULT SampleTransform::AddInputStreams(
    DWORD   cStreams,
    DWORD*  adwStreamIDs)
{
    return E_NOTIMPL;
}



//
// Return one of the preferred input media types for this MFT, specified by
// media type index and by stream ID.
//
HRESULT SampleTransform::GetInputAvailableType(
    DWORD           dwInputStreamID,
    DWORD           dwTypeIndex,
    IMFMediaType    **ppType)
{
    HRESULT hr = S_OK;
    CComPtr<IMFMediaType> pmt;

    do
    {
        CComCritSecLock<CComAutoCriticalSection> lock(m_critSec);

        BREAK_ON_NULL(ppType, E_POINTER);

        // only a single stream is supported
        if (dwInputStreamID != 0)
        {
            hr = MF_E_INVALIDSTREAMNUMBER;
            BREAK_ON_FAIL(hr);
        }

        // If the output is not set, then return one of the supported media types.
        // Otherwise return the media type previously set.
        if (m_pOutputType == NULL)
        {
            hr = GetSupportedMediaType(dwTypeIndex, &pmt);
            BREAK_ON_FAIL(hr);

            // return the resulting media type
            *ppType = pmt.Detach();
        }
        else if (dwTypeIndex == 0)
        {
            // return the set output type
            *ppType = m_pOutputType;
            (*ppType)->AddRef();
        }
        else
        {
            // if the output type is set, the MFT supports only one input type, and the 
            // index cannot be more than 0
            hr = MF_E_NO_MORE_TYPES;
        }
    } while (false);

    if (FAILED(hr) && ppType != NULL)
    {
        *ppType = NULL;
    }

    return hr;
}



//
// Description: Return a preferred output type.
//
HRESULT SampleTransform::GetOutputAvailableType(
    DWORD           dwOutputStreamID,
    DWORD           dwTypeIndex, // 0-based
    IMFMediaType    **ppType)
{
    HRESULT hr = S_OK;
    CComPtr<IMFMediaType> pmt;

    do
    {
        // lock the MFT - the lock will disengage when variable goes out of scope
        CComCritSecLock<CComAutoCriticalSection> lock(m_critSec);

        BREAK_ON_NULL(ppType, E_POINTER);

        if (dwOutputStreamID != 0)
        {
            hr = MF_E_INVALIDSTREAMNUMBER;
            BREAK_ON_FAIL(hr);
        }

        // If the input is not set, then return one of the supported media types.
        // Otherwise return the media type previously set.
        if (m_pInputType == NULL)
        {
            hr = GetSupportedMediaType(dwTypeIndex, &pmt);
            BREAK_ON_FAIL(hr);

            // return the resulting media type
            *ppType = pmt;
            (*ppType)->AddRef();
        }
        else
        {
            *ppType = m_pInputType;
            (*ppType)->AddRef();
        }
    } while (false);

    return hr;
}



//
// Set, test, or clear the input media type for the MFT.
//
HRESULT SampleTransform::SetInputType(DWORD dwInputStreamID, IMFMediaType* pType,
    DWORD dwFlags)
{
    HRESULT hr = S_OK;
    CComPtr<IMFAttributes> pTypeAttributes = pType;

    do
    {
        // lock the MFT - the lock will disengage when variable goes out of scope
        CComCritSecLock<CComAutoCriticalSection> lock(m_critSec);

        // this MFT supports only a single stream - fail if a different stream value is
        // suggested
        if (dwInputStreamID != 0)
        {
            hr = MF_E_INVALIDSTREAMNUMBER;
            BREAK_ON_FAIL(hr);
        }

        // verify that the specified media type is acceptible to the MFT
        hr = CheckMediaType(pType);
        BREAK_ON_FAIL(hr);

        // If the MFT is already processing a sample internally, fail out, since the MFT
        // can't change formats on the fly.
        if (m_pSample != NULL)
        {
            hr = MF_E_TRANSFORM_CANNOT_CHANGE_MEDIATYPE_WHILE_PROCESSING;
            BREAK_ON_FAIL(hr);
        }

        // Make sure that the input media type is the same as the output type (if the 
        // output is set).  If the type passed in is NULL, skip this check since NULL
        // clears the type.
        if (pType != NULL && m_pOutputType != NULL)
        {
            BOOL result = FALSE;
            hr = pType->Compare(pTypeAttributes, MF_ATTRIBUTES_MATCH_INTERSECTION, &result);
            BREAK_ON_FAIL(hr);

            // if the types don't match, return an error code
            if (!result)
            {
                hr = MF_E_INVALIDMEDIATYPE;
                break;
            }
        }

        // If we got here, then the media type is acceptable - set it if the caller
        // explicitly tells us to set it, and is not just checking for compatible
        // types.
        if (dwFlags != MFT_SET_TYPE_TEST_ONLY)
        {
            m_pInputType = pType;
        }
    } while (false);

    return hr;
}



//
// Set, test, or clear the output media type of the MFT
//
HRESULT SampleTransform::SetOutputType(
    DWORD           dwOutputStreamID,
    IMFMediaType*   pmt, // Can be NULL to clear the output type.
    DWORD           dwFlags)
{
    HRESULT hr = S_OK;

    CComPtr<IMFMediaType> pType = pmt;

    do
    {
        // lock the MFT - the lock will disengage when variable goes out of scope
        CComCritSecLock<CComAutoCriticalSection> lock(m_critSec);

        // this MFT supports only a single stream - fail if a different stream value
        // is suggested
        if (dwOutputStreamID != 0)
        {
            hr = MF_E_INVALIDSTREAMNUMBER;
            BREAK_ON_FAIL(hr);
        }

        // verify that the specified media type is acceptible to the MFT
        hr = CheckMediaType(pType);
        BREAK_ON_FAIL(hr);

        // If the MFT is already processing a sample internally, fail out, since the
        // MFT can't change formats on the fly.
        if (m_pSample != NULL)
        {
            hr = MF_E_TRANSFORM_CANNOT_CHANGE_MEDIATYPE_WHILE_PROCESSING;
            BREAK_ON_FAIL(hr);
        }

        // Make sure that the media type is not NULL, and that the input media type
        // is the same as the output type (if the input is set).
        if (pType != NULL && m_pInputType != NULL)
        {
            DWORD flags = 0;
            hr = pType->IsEqual(m_pInputType, &flags);
            BREAK_ON_FAIL(hr);
        }

        // If we got here, then the media type is acceptable - set it if the caller
        // explicitly tells us to set it, and is not just checking for compatible
        // types.
        if (dwFlags != MFT_SET_TYPE_TEST_ONLY)
        {
            m_pOutputType = pType;
        }
    } while (false);


    return hr;
}



// 
// Get the current input media type.
//
HRESULT SampleTransform::GetInputCurrentType(
    DWORD           dwInputStreamID,
    IMFMediaType**  ppType)
{
    HRESULT hr = S_OK;

    do
    {
        // lock the MFT
        CComCritSecLock<CComAutoCriticalSection> lock(m_critSec);

        BREAK_ON_NULL(ppType, E_POINTER);

        if (dwInputStreamID != 0)
        {
            hr = MF_E_INVALIDSTREAMNUMBER;
        }
        else if (m_pInputType == NULL)
        {
            hr = MF_E_TRANSFORM_TYPE_NOT_SET;
        }
        else
        {
            *ppType = m_pInputType;
            (*ppType)->AddRef();
        }
    } while (false);

    return hr;
}



//
// Get the current output type
//
HRESULT SampleTransform::GetOutputCurrentType(
    DWORD           dwOutputStreamID,
    IMFMediaType**  ppType)
{
    HRESULT hr = S_OK;

    CComCritSecLock<CComAutoCriticalSection> lock(m_critSec);

    if (ppType == NULL)
    {
        return E_POINTER;
    }

    // verify the correct output stream ID and that the output
    // type has been set
    if (dwOutputStreamID != 0)
    {
        hr = MF_E_INVALIDSTREAMNUMBER;
    }
    else if (m_pOutputType == NULL)
    {
        hr = MF_E_TRANSFORM_TYPE_NOT_SET;
    }
    else
    {
        *ppType = m_pOutputType;
        (*ppType)->AddRef();
    }

    return hr;
}



//
// Check to see if the MFT is ready to accept input samples
//
HRESULT SampleTransform::GetInputStatus(
    DWORD           dwInputStreamID,
    DWORD*          pdwFlags)
{
    CComCritSecLock<CComAutoCriticalSection> lock(m_critSec);

    if (pdwFlags == NULL)
    {
        return E_POINTER;
    }

    // the MFT supports only a single stream.
    if (dwInputStreamID != 0)
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    // if there is no sample queued in the MFT, it is ready to accept data. If there already 
    // is a sample in the MFT, the MFT can't accept any more samples until somebody calls  
    // ProcessOutput to extract that sample, or flushes the MFT.
    if (m_pSample == NULL)
    {
        // there is no sample in the MFT - ready to accept data
        *pdwFlags = MFT_INPUT_STATUS_ACCEPT_DATA;
    }
    else
    {
        // a value of zero indicates that the MFT can't accept any more data
        *pdwFlags = 0;
    }

    return S_OK;
}



//
// Get the status of the output stream of the MFT - IE verify whether there
// is a sample ready in the MFT.  This method can be left unimplemented.
//
HRESULT SampleTransform::GetOutputStatus(DWORD* pdwFlags)
{
    return E_NOTIMPL;
}



//
// Set the range of time stamps that the MFT will output.  This MFT does
// not implement this behavior, and is left unimplemented.
//
HRESULT SampleTransform::SetOutputBounds(
    LONGLONG        hnsLowerBound,
    LONGLONG        hnsUpperBound)
{
    return E_NOTIMPL;
}



//
// Send an event to an input stream.  Since this MFT does not handle any
// such commands, this method is left unimplemented.
//
HRESULT SampleTransform::ProcessEvent(
    DWORD              dwInputStreamID,
    IMFMediaEvent*     pEvent)
{

    return E_NOTIMPL;
}



//
// Receive and process a message or command to the MFT, specifying a
// requested behavior.
//
HRESULT SampleTransform::ProcessMessage(
    MFT_MESSAGE_TYPE    eMessage,
    ULONG_PTR           ulParam)
{
    HRESULT hr = S_OK;

    CComCritSecLock<CComAutoCriticalSection> lock(m_critSec);

    if (eMessage == MFT_MESSAGE_COMMAND_FLUSH)
    {
        // Flush the MFT - release all samples in it and reset the state
        m_pSample = NULL;
    }
    else if (eMessage == MFT_MESSAGE_COMMAND_DRAIN)
    {
        // The drain command tells the MFT not to accept any more input until
        // all of the pending output has been processed. That is the default 
        // behavior of this MFT, so there is nothing to do.
    }
    else if (eMessage == MFT_MESSAGE_NOTIFY_BEGIN_STREAMING)
    {
    }
    else if (eMessage == MFT_MESSAGE_NOTIFY_END_STREAMING)
    {
    }
    else if (eMessage == MFT_MESSAGE_NOTIFY_END_OF_STREAM)
    {
    }
    else if (eMessage == MFT_MESSAGE_NOTIFY_START_OF_STREAM)
    {
    }

    return hr;
}



//
// Receive and process an input sample.
//
HRESULT SampleTransform::ProcessInput(
    DWORD               dwInputStreamID,
    IMFSample*          pSample,
    DWORD               dwFlags)
{
    HRESULT hr = S_OK;
    DWORD dwBufferCount = 0;

    do
    {
        // lock the MFT
        CComCritSecLock<CComAutoCriticalSection> lock(m_critSec);

        BREAK_ON_NULL(pSample, E_POINTER);

        // This MFT accepts only a single output sample at a time, and does not accept any
        // flags.
        if (dwInputStreamID != 0 || dwFlags != 0)
        {
            hr = E_INVALIDARG;
            break;
        }

        // Both input and output media types must be set in order for the MFT to function.
        BREAK_ON_NULL(m_pInputType, MF_E_NOTACCEPTING);
        BREAK_ON_NULL(m_pOutputType, MF_E_NOTACCEPTING);

        // The MFT already has a sample that has not yet been processed.
        if (m_pSample != NULL)
        {
            hr = MF_E_NOTACCEPTING;
            break;
        }

        // Store the sample for later processing.
        m_pSample = pSample;
    } while (false);


    return hr;
}



//
// Get an output sample from the MFT.
//
HRESULT SampleTransform::ProcessOutput(
    DWORD                   dwFlags,
    DWORD                   cOutputBufferCount,
    MFT_OUTPUT_DATA_BUFFER* pOutputSampleBuffer,
    DWORD*                  pdwStatus)
{
    HRESULT hr = S_OK;

    // lock the MFT
    CComCritSecLock<CComAutoCriticalSection> lock(m_critSec);


    // This MFT accepts only a single output sample at a time, and does
    // not accept any flags.
    if (cOutputBufferCount != 1 || dwFlags != 0)
    {
        return E_INVALIDARG;
    }

    // If we don't have an input sample, we need some input before
    // we can generate any output - return a flag indicating that more 
    // input is needed.
    if (m_pSample == NULL) {
        return MF_E_TRANSFORM_NEED_MORE_INPUT;
    }



    CComPtr<IMFSample> pSample = m_pSample.Detach();

    LONGLONG sampleTimestamp, duration;
    pSample->GetSampleDuration(&duration);

    int bufferLenght = 100;
    wchar_t charArr[100];

    swprintf_s(charArr, bufferLenght, L"%llu", duration);
    OutputDebugStringW(L"------------ Sample duration: ");
    OutputDebugStringW(charArr);
    OutputDebugStringW(L"\n");

    swprintf_s(charArr, bufferLenght, L"%llu", m_PrevTimestamp);
    OutputDebugStringW(L"------------ Sample previous timeStamp: ");
    OutputDebugStringW(charArr);
    OutputDebugStringW(L"\n");

    pSample->GetSampleTime(&sampleTimestamp);
  //  sampleTimestamp = sampleTimestamp - 853875948893;
    pSample->SetUINT64(MFSampleExtension_DeviceTimestamp, sampleTimestamp - 853875948893);
    swprintf_s(charArr, bufferLenght, L"%llu", sampleTimestamp);
    OutputDebugStringW(L"------------ Sample timeStamp: ");
    OutputDebugStringW(charArr);
    OutputDebugStringW(L"\n");

    swprintf_s(charArr, bufferLenght, L"%llu", sampleTimestamp - m_PrevTimestamp);
    OutputDebugStringW(L"------------ Difference: ");
    OutputDebugStringW(charArr);
    OutputDebugStringW(L"\n\n");

    if (!m_FirstSampleTimestampInitialized)
    {
        m_FirstSampleTimestamp = sampleTimestamp;
        m_FirstSampleTimestampInitialized = true;
    }

    //int bufferLenght = 100;
    //wchar_t charArr[100];

    //swprintf_s(charArr, bufferLenght, L"%llu", duration);
    //OutputDebugStringW(L"------------ Sample duration: ");
    //OutputDebugStringW(charArr);
    //OutputDebugStringW(L"\n");

    //swprintf_s(charArr, bufferLenght, L"%llu", sampleTimestamp);
    //OutputDebugStringW(L"------------ Sample old timestamp: ");
    //OutputDebugStringW(charArr);
    //OutputDebugStringW(L"\n");

    //swprintf_s(charArr, bufferLenght, L"%llu", sampleTimestamp + duration);
    //OutputDebugStringW(L"------------ Sample old timestamp calculated nextTImestamp: ");
    //OutputDebugStringW(charArr);
    //OutputDebugStringW(L"\n");

    pSample->SetSampleTime(sampleTimestamp - m_FirstSampleTimestamp);
    m_PrevTimestamp = sampleTimestamp;
    //swprintf_s(charArr, bufferLenght, L"%llu", m_timeOffset);
    //OutputDebugStringW(L"------------ Sample new timestamp: ");
    //OutputDebugStringW(charArr);
    //OutputDebugStringW(L"\n");

    //swprintf_s(charArr, bufferLenght, L"%llu", m_timeOffset + duration);
    //OutputDebugStringW(L"------------ Sample new timestamp calculated nextTimestamp: ");
    //OutputDebugStringW(charArr);
    //OutputDebugStringW(L"\n\n");
    //m_timeOffset += duration;


   
    // Detach the output sample from the MFT and put the pointer for
    // the processed sample into the output buffer
    pOutputSampleBuffer[0].pSample = pSample;

    // Set status flags for output
    pOutputSampleBuffer[0].dwStatus = 0;

    *pdwStatus = 0;

    return hr;
}


//
// Construct and return a partial media type with the specified index from the list of media
// types supported by this MFT.
//
HRESULT SampleTransform::GetSupportedMediaType(
    DWORD           dwTypeIndex,
    IMFMediaType**  ppMT)
{
    HRESULT hr = S_OK;
    CComPtr<IMFMediaType> pmt;

    do
    {
        // create a new media type object
        hr = MFCreateMediaType(&pmt);
        BREAK_ON_FAIL(hr);

        // set the major type of the media type to video
        hr = pmt->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        BREAK_ON_FAIL(hr);

        // set the subtype of the video type by index.  The indexes of the media types
        // that are supported by this filter are:  0 - UYVY, 1 - NV12
        if (dwTypeIndex == 0)
        {
            hr = pmt->SetGUID(MF_MT_SUBTYPE, MEDIASUBTYPE_H264);
        }
        else if (dwTypeIndex == 1)
        {
            hr = pmt->SetGUID(MF_MT_SUBTYPE, MEDIASUBTYPE_RGB24);
        }
        else if (dwTypeIndex == 2)
        {
            hr = pmt->SetGUID(MF_MT_SUBTYPE, MEDIASUBTYPE_RGB32);
        }
        else if (dwTypeIndex == 3)
        {
            hr = pmt->SetGUID(MF_MT_SUBTYPE, MEDIASUBTYPE_YUY2);
        }
        else if (dwTypeIndex == 4) {
            hr = pmt->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_IYUV);
        }
        else if (dwTypeIndex == 5)
        {
            hr = pmt->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB24);
        }
        else
        {
            // if we don't have any more media types, return an error signifying
            // that there is no media type with that index
            hr = MF_E_NO_MORE_TYPES;
        }
        BREAK_ON_FAIL(hr);

        // detach the underlying IUnknown pointer from the pmt CComPtr without
        // releasing the pointer so that we can return that object to the caller.
        *ppMT = pmt.Detach();
    } while (false);

    return hr;
}






//
// Description: Validates a media type for this transform.
//
HRESULT SampleTransform::CheckMediaType(IMFMediaType* pmt)
{
    GUID majorType = GUID_NULL;
    GUID subtype = GUID_NULL;
    MFVideoInterlaceMode interlacingMode = MFVideoInterlace_Unknown;
    HRESULT hr = S_OK;

    // store the media type pointer in the CComPtr so that it's reference counter
    // is incremented and decrimented properly.
    CComPtr<IMFMediaType> pType = pmt;

    do
    {
        BREAK_ON_NULL(pType, E_POINTER);

        // Extract the major type to make sure that the major type is video
        hr = pType->GetGUID(MF_MT_MAJOR_TYPE, &majorType);
        BREAK_ON_FAIL(hr);

        if (majorType != MFMediaType_Video)
        {
            hr = MF_E_INVALIDMEDIATYPE;
            break;
        }


        // Extract the subtype to make sure that the subtype is one that we support
        hr = pType->GetGUID(MF_MT_SUBTYPE, &subtype);
        BREAK_ON_FAIL(hr);

        // verify that the specified media type has one of the acceptable subtypes -
        // this filter will accept only NV12 and UYVY uncompressed subtypes.
        if (subtype != MEDIASUBTYPE_H264 && subtype != MFVideoFormat_YV12 && subtype != MFVideoFormat_RGB24 && subtype != MEDIASUBTYPE_RGB24 && subtype != MEDIASUBTYPE_RGB32 && subtype != MEDIASUBTYPE_YUY2 && subtype != MFVideoFormat_IYUV)
        {

            hr = MF_E_INVALIDMEDIATYPE;
            break;
        }

    } while (false);

    return hr;
}