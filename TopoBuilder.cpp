#include "TopoBuilder.h"
#include "SampleTransform.h"
#include <Wmcodecdsp.h>

IMFTransform* CreateColorConverterMFT()
{
    HRESULT hr = S_OK;
    //register color converter locally
    hr = MFTRegisterLocalByCLSID(__uuidof(CColorConvertDMO), MFT_CATEGORY_VIDEO_PROCESSOR, L"", MFT_ENUM_FLAG_SYNCMFT, 0, NULL, 0, NULL);
    THROW_ON_FAIL(hr);
    //create color converter
    IMFTransform *pColorConverterMFT = NULL;
    hr = CoCreateInstance(__uuidof(CColorConvertDMO), NULL, CLSCTX_INPROC_SERVER, IID_IMFTransform, (void**)&pColorConverterMFT);
    THROW_ON_FAIL(hr);

    return pColorConverterMFT;
}

IMFTransform* CreateEncoderMft(IMFMediaType * inputMediaType, GUID outMajorType, GUID outSubType)
{
    CComPtr<IMFTransform> pEncoder = FindEncoderTransform(outMajorType, outSubType);
    HRESULT hr = S_OK;

    GUID major = GetMajorType(inputMediaType);
    CComPtr<IMFMediaType> outputType = CreateMediaType(outMajorType, outSubType);
    hr = CopyVideoType(inputMediaType, outputType);
    THROW_ON_FAIL(hr);

    hr = pEncoder->SetOutputType(0, outputType.Detach(), 0);
    THROW_ON_FAIL(hr);

    DWORD mftStatus = 0;
    pEncoder->GetInputStatus(0, &mftStatus);
    if (MFT_INPUT_STATUS_ACCEPT_DATA != mftStatus) {
        THROW_ON_FAIL(S_FALSE);
    }
    return pEncoder.Detach();
}

//
// Initiates topology building from the file URL by first creating a media source, and then
// adding source and sink nodes for every stream found in the file.
//
HRESULT CTopoBuilder::RenderURL(PCWSTR fileUrl, TopologySettings topoSettings)
{
    HRESULT hr = S_OK;

    do
    {
        m_topoSettings = topoSettings;
        m_videoHwnd = topoSettings.hWnd;

        // The topology can have either a rendering sink (when videoHwnd is not NULL), a 
        // network sink, or both.
        if(m_videoHwnd == NULL && !topoSettings.addNetwork)
        {
            hr = E_INVALIDARG;
            break;
        }

        // first create the media source for the file/stream passed in.  Fail and fall out if
        // the media source creation fails (e.g. if the file format is not recognized)
        hr = CreateMediaSource(fileUrl);
        BREAK_ON_FAIL(hr);

        // add a network sink if one was requested
        if(topoSettings.addNetwork)
        {
            hr = CreateNetworkSink(8080);
            BREAK_ON_FAIL(hr);
        }

        // create the actual topology
        hr = CreateTopology();
    }
    while(false);

    return hr;
}

HRESULT CTopoBuilder::RenderCamera(TopologySettings topoSettings)
{
    HRESULT hr = S_OK;

    do
    {
        m_topoSettings = topoSettings;
        m_videoHwnd = topoSettings.hWnd;
        // The topology can have either a rendering sink (when videoHwnd is not NULL), a 
        // network sink, or both.
        if (m_videoHwnd == NULL && !topoSettings.addNetwork)
        {
            hr = E_INVALIDARG;
            break;
        }

        // first create the media source for the file/stream passed in.  Fail and fall out if
        // the media source creation fails (e.g. if the file format is not recognized)
        hr = CreateMediaSource();
        BREAK_ON_FAIL(hr);

        // add a network sink if one was requested
        if (topoSettings.addNetwork)
        {
            hr = CreateNetworkSink(8080);
            BREAK_ON_FAIL(hr);
        }

        // create the actual topology
        hr = CreateTopology();
    } while (false);

    return hr;
}
HRESULT CTopoBuilder::CreateMediaTypeForAsfProfile(IMFMediaType** ppMediaType)
{
    HRESULT hr = S_OK;
    CComPtr<IMFMediaType> sourceMediaType;
    CComPtr<IMFPresentationDescriptor> pPresDescriptor;
    CComPtr<IMFStreamDescriptor> pStreamDescriptor;
    CComPtr<IMFMediaTypeHandler> pHandler = NULL;

    BOOL selected;
    // create the presentation descriptor for the source
    hr = m_pSource->CreatePresentationDescriptor(&pPresDescriptor);
    BREAK_ON_FAIL(hr);
    hr = pPresDescriptor->GetStreamDescriptorByIndex(0, &selected, &pStreamDescriptor);
    BREAK_ON_FAIL(hr);
    hr = pStreamDescriptor->GetMediaTypeHandler(&pHandler);
    BREAK_ON_FAIL(hr);
    hr = pHandler->GetCurrentMediaType(&sourceMediaType);
    BREAK_ON_FAIL(hr);

    
    if (m_topoSettings.addH264Encoder)
    {
        CComPtr<IMFMediaType> h264MediaType = CreateMediaType(MFMediaType_Video, MFVideoFormat_H264);
        CopyVideoType(sourceMediaType, h264MediaType);
        *ppMediaType = h264MediaType.Detach();
    }
    else if (m_topoSettings.addWMWEncoder)
    {
        CComPtr<IMFMediaType> wmwEncoderType = CreateMediaType(MFMediaType_Video, MFVideoFormat_WMV3);
        CopyVideoType(sourceMediaType, wmwEncoderType);
        *ppMediaType = wmwEncoderType.Detach();
    }
    else // Raw mediatype
    {
        *ppMediaType = sourceMediaType.Detach();
    }
}

HRESULT CTopoBuilder::CreateASFProfile(IMFASFProfile** ppAsfProfile)
{
    /////////////////////////// 
    //[ORIGINAL IMPLEMENTATION]
    /////////////////////////// 
    // create the ASF profile from the presentation descriptor
    //hr = MFCreateASFProfileFromPresentationDescriptor(pPresDescriptor, &pAsfProfile);
    //BREAK_ON_FAIL(hr);

    HRESULT hr = S_OK;
    
    CComPtr<IMFMediaType> pMediaType;
    CComPtr<IMFASFProfile> pNewASFProfile;
    CComPtr<IMFASFStreamConfig> pASFStreamConfig;
    do
    {
        hr = CreateMediaTypeForAsfProfile(&pMediaType);
        BREAK_ON_FAIL(hr);

        hr = MFCreateASFProfile(&pNewASFProfile);
        BREAK_ON_FAIL(hr);

        hr = pNewASFProfile->CreateStream(pMediaType, &pASFStreamConfig);
        BREAK_ON_FAIL(hr);

        hr = pASFStreamConfig->SetMediaType(pMediaType);
        BREAK_ON_FAIL(hr);

        hr = pASFStreamConfig->SetStreamNumber(1);
        BREAK_ON_FAIL(hr);
        hr = pNewASFProfile->SetStream(pASFStreamConfig);
        BREAK_ON_FAIL(hr);
    } while (false);

    *ppAsfProfile = pNewASFProfile.Detach();
    return hr;
}

//
// Create a network sink that will listen for requests on the specified port.
//
HRESULT CTopoBuilder::CreateNetworkSink(DWORD requestPort)
{
    HRESULT hr = S_OK;
    
    CComPtr<IMFASFProfile> pAsfProfile;
    CComQIPtr<IMFASFContentInfo> pAsfContentInfo;
    CComPtr<IMFActivate> pByteStreamActivate;
    CComPtr<IMFActivate> pNetSinkActivate;

    do
    {
        BREAK_ON_NULL(m_pSource, E_UNEXPECTED);

        // create an HTTP activator for the custom HTTP output byte stream object
        pByteStreamActivate = new (std::nothrow) CHttpOutputStreamActivate(requestPort);
        BREAK_ON_NULL(pByteStreamActivate, E_OUTOFMEMORY);

        hr = CreateASFProfile(&pAsfProfile);
        BREAK_ON_FAIL(hr);

        // create the ContentInfo object for the ASF profile
        hr = MFCreateASFContentInfo(&pAsfContentInfo);
        BREAK_ON_FAIL(hr);

        // set the profile on the content info object
        hr = pAsfContentInfo->SetProfile(pAsfProfile);
        BREAK_ON_FAIL(hr);

        // create an activator object for an ASF streaming sink
        hr = MFCreateASFStreamingMediaSinkActivate(pByteStreamActivate, pAsfContentInfo, 
            &m_pNetworkSinkActivate);
        BREAK_ON_FAIL(hr);
    }
    while(false);

    return hr;
}


//
// Create a media source for the specified URL string.  The URL can be a path to a stream, 
// or it can be a path to a local file.
//
HRESULT CTopoBuilder::CreateMediaSource(PCWSTR sURL)
{
    HRESULT hr = S_OK;
    MF_OBJECT_TYPE objectType = MF_OBJECT_INVALID;
    CComPtr<IMFSourceResolver> pSourceResolver = NULL;
    CComPtr<IUnknown> pSource;

    do
    {
        // Create the source resolver.
        hr = MFCreateSourceResolver(&pSourceResolver);
        BREAK_ON_FAIL(hr);

        // Use the syncrhonous source resolver to create the media source.
        hr = pSourceResolver->CreateObjectFromURL(
            sURL,                       // URL of the source.
            MF_RESOLUTION_MEDIASOURCE | 
                MF_RESOLUTION_CONTENT_DOES_NOT_HAVE_TO_MATCH_EXTENSION_OR_MIME_TYPE,  
                                        // indicate that we want a source object, and 
                                        // pass in optional source search parameters
            NULL,                       // Optional property store for extra parameters
            &objectType,                // Receives the created object type.
            &pSource                    // Receives a pointer to the media source.
            );
        BREAK_ON_FAIL(hr);

        // Get the IMFMediaSource interface from the media source.
        m_pSource = pSource;
        BREAK_ON_NULL(m_pSource, E_NOINTERFACE);
    }
    while(false);

    return hr;
}

HRESULT CTopoBuilder::CreateMediaSource()
{
    IMFActivate **ppDevices;
    UINT32 count;
    HRESULT hr = S_OK;
    MF_OBJECT_TYPE objectType = MF_OBJECT_INVALID;
    CComPtr<IUnknown> pSource;
    IMFAttributes *pAttributes = NULL;

    do
    {
        hr = MFCreateAttributes(&pAttributes, 2);

        BREAK_ON_FAIL(hr);

        // Ask for source type = video capture devices.

        hr = pAttributes->SetGUID(
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
        );
        hr = pAttributes->SetUINT32(
            MFT_HW_TIMESTAMP_WITH_QPC_Attribute,
            true
        );

        hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
        BREAK_ON_FAIL(hr);
        hr = ppDevices[0]->ActivateObject(
            __uuidof(IMFMediaSource),
            (void**)&pSource
        );
        BREAK_ON_FAIL(hr);

        // Get the IMFMediaSource interface from the media source.
        m_pSource = pSource;
    }
    while(false);
    // BREAK_ON_NULL(m_pSource, E_NOINTERFACE);

    return hr;
}



//
// Since we created the source, we are responsible for shutting it down.
//
HRESULT CTopoBuilder::ShutdownSource(void)
{    
    HRESULT hr = S_OK;

    if(m_pSource != NULL)
    {
        // shut down the source
        hr = m_pSource->Shutdown();
        
        // release the source, since all subsequent calls to it will fail
        m_pSource.Release();
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    return hr;
}



//
//  Creates a playback topology from the media source by extracting presentation
// and stream descriptors from the source, and creating a sink for each of them.
//
HRESULT CTopoBuilder::CreateTopology(void)
{
    HRESULT hr = S_OK;
    CComQIPtr<IMFPresentationDescriptor> pPresDescriptor;
    DWORD nSourceStreams = 0;

    do
    {
        // release the old topology if there was one        
        m_pTopology.Release();
        
        // Create a new topology.
        hr = MFCreateTopology(&m_pTopology);
        BREAK_ON_FAIL(hr);

        // Create the presentation descriptor for the media source - a container object that
        // holds a list of the streams and allows selection of streams that will be used.
        hr = m_pSource->CreatePresentationDescriptor(&pPresDescriptor);

        BREAK_ON_FAIL(hr);

        // Get the number of streams in the media source
        hr = pPresDescriptor->GetStreamDescriptorCount(&nSourceStreams);
        BREAK_ON_FAIL(hr);

        // For each stream, create source and sink nodes and add them to the topology.
        for (DWORD x = 0; x < nSourceStreams; x++)
        {
            hr = AddBranchToPartialTopology(pPresDescriptor, x);
            
            // if we failed to build a branch for this stream type, then deselect it
            // that will cause the stream to be disabled, and the source will not produce
            // any data for it
            if(FAILED(hr))
            {
                hr = pPresDescriptor->DeselectStream(x);
                BREAK_ON_FAIL(hr);
            }
        }
    }
    while(false);

    return hr;
}


//
//  Adds a topology branch for one stream.
//
//  pPresDescriptor: The source's presentation descriptor.
//  nStream: Index of the stream to render.
//
//  For each stream, we must do the following steps:
//    1. Create a source node associated with the stream.
//    2. Create an output node for the renderer.
//    3. Connect the two nodes.
//  The media session will resolve the topology, inserting intermediate decoder and other 
//  transform MFTs that will process the data in preparation for consumption by the renderers.
//
HRESULT CTopoBuilder::AddBranchToPartialTopology(
    CComPtr<IMFPresentationDescriptor> pPresDescriptor, 
    DWORD nStream)
{
    HRESULT hr = S_OK;
    CComPtr<IMFStreamDescriptor> pStreamDescriptor;
    CComPtr<IMFTopologyNode> pSourceNode;
    CComPtr<IMFTopologyNode> pOutputNode;
    BOOL streamSelected = FALSE;

    do
    {
        BREAK_ON_NULL(m_pTopology, E_UNEXPECTED);

        // Get the stream descriptor for this stream (information about stream).
        hr = pPresDescriptor->GetStreamDescriptorByIndex(nStream, &streamSelected, &pStreamDescriptor);
        BREAK_ON_FAIL(hr);

        // Create the topology branch only if the stream is selected - IE if the user wants to play it.
        if (streamSelected)
        {
            // Create a source node for this stream.
            hr = CreateSourceStreamNode(pPresDescriptor, pStreamDescriptor, pSourceNode);
            BREAK_ON_FAIL(hr);
            m_pSourceNode = pSourceNode;
            // Create the output node for the renderer.
            hr = CreateOutputNode(pStreamDescriptor, m_videoHwnd, pSourceNode, &pOutputNode);
            BREAK_ON_FAIL(hr);
         

            // Add the source and sink nodes to the topology.
            hr = m_pTopology->AddNode(pSourceNode);
            BREAK_ON_FAIL(hr);

            hr = m_pTopology->AddNode(pOutputNode);
            THROW_ON_FAIL(hr);
            

            // Connect the source node to the output node.  The topology will find the
            // intermediate nodes needed to convert media types.
            hr = pSourceNode->ConnectOutput(0, pOutputNode, 0);

            UnwrapPartialTopo(pSourceNode, 0);
        }
    }
    while(false);

    return hr;
}



//
//  Create a source node for the specified stream
//
//  pPresDescriptor: Presentation descriptor for the media source.
//  pStreamDescriptor: Stream descriptor for the stream.
//  pNode: Reference to a pointer to the new node - returns the new node.
//
HRESULT CTopoBuilder::CreateSourceStreamNode(
    CComPtr<IMFPresentationDescriptor> pPresDescriptor,
    CComPtr<IMFStreamDescriptor> pStreamDescriptor,
    CComPtr<IMFTopologyNode> &pNode)
{
    HRESULT hr = S_OK;

    do
    {
        BREAK_ON_NULL(pPresDescriptor, E_POINTER);
        BREAK_ON_NULL(pStreamDescriptor, E_POINTER);

        // Create the topology node, indicating that it must be a source node.
        hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &pNode);
        BREAK_ON_FAIL(hr);

        // Associate the node with the source by passing in a pointer to the media source,
        // and indicating that it is the source
        hr = pNode->SetUnknown(MF_TOPONODE_SOURCE, m_pSource);
        BREAK_ON_FAIL(hr);

        // Set the node presentation descriptor attribute of the node by passing 
        // in a pointer to the presentation descriptor
        hr = pNode->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, pPresDescriptor);
        BREAK_ON_FAIL(hr);

        // Set the node stream descriptor attribute by passing in a pointer to the stream
        // descriptor
        hr = pNode->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, pStreamDescriptor);
        BREAK_ON_FAIL(hr);
    }
    while(false);

    return hr;
}


HRESULT AddOldOutputToTopo(
    IMFTopology *pTopology,     // Topology.
    IMFTransform *pMFT,         // MFT.
    IMFTopologyNode *output,
    IMFTopologyNode **ppNode    // Receives the node pointer.
)
{
    *ppNode = NULL;

    CComPtr<IMFTopologyNode> pNode = NULL;

    // Create the node.
    HRESULT hr = MFCreateTopologyNode(MF_TOPOLOGY_TRANSFORM_NODE, &pNode);
    THROW_ON_FAIL(hr);

    // Set the object pointer.
    hr = pNode->SetObject(pMFT);
    THROW_ON_FAIL(hr);

    hr = pNode->SetUINT32(MF_TOPONODE_STREAMID, 0);
    THROW_ON_FAIL(hr);

    hr = pNode->ConnectOutput(0, output, 0);
    THROW_ON_FAIL(hr);

    // Add the node to the topology.
    hr = pTopology->AddNode(output);
    THROW_ON_FAIL(hr);

    // Return the pointer to the caller.

    *ppNode = pNode;
    (*ppNode)->AddRef();

    return hr;
}

//
//  This function creates an output node for a stream (sink) by going through the
//  following steps:
//  1. Select a renderer based on the media type of the stream - EVR or SAR.
//  2. Create an IActivate object for the renderer.
//  3. Create an output topology node.
//  4. Put the IActivate pointer in the node.
//
//  pStreamDescriptor: pointer to the descriptor for the stream that we are working
//  with.
//  hwndVideo: handle to the video window used if this is the video stream.  If this is
//  an audio stream this parameter is not used.
//  pNode: reference to a pointer to the new node.
//
HRESULT CTopoBuilder::CreateOutputNode(
    CComPtr<IMFStreamDescriptor> pStreamDescriptor,
    HWND hwndVideo,
    IMFTopologyNode* pSNode,
    IMFTopologyNode** ppOutputNode)
{
    HRESULT hr = S_OK;
    CComPtr<IMFMediaTypeHandler> pHandler = NULL;
    CComPtr<IMFActivate> pRendererActivate = NULL;
    CComPtr<IMFTopologyNode> pSourceNode = pSNode;
    CComPtr<IMFTopologyNode> pOutputNode;

    GUID majorType = GUID_NULL;

    do
    {
        if(m_videoHwnd != NULL)
        {
            // Get the media type handler for the stream which will be used to process
            // the media types of the stream.  The handler stores the media type.
            hr = pStreamDescriptor->GetMediaTypeHandler(&pHandler);
            BREAK_ON_FAIL(hr);

            // Get the major media type (e.g. video or audio)
            hr = pHandler->GetMajorType(&majorType);
            BREAK_ON_FAIL(hr);

            // Create an IMFActivate controller object for the renderer, based on the media type.
            // The activation objects are used by the session in order to create the renderers only when 
            // they are needed - IE only right before starting playback.  The activation objects are also
            // used to shut down the renderers.
            if (majorType == MFMediaType_Audio)
            {
                // if the stream major type is audio, create the audio renderer.
                hr = MFCreateAudioRendererActivate(&pRendererActivate);
            }
            else if (majorType == MFMediaType_Video)
            {
                // if the stream major type is video, create the video renderer, passing in the video
                // window handle - that's where the video will be playing.
                hr = MFCreateVideoRendererActivate(hwndVideo, &pRendererActivate);
            }
            else
            {
                // fail if the stream type is not video or audio.  For example fail
                // if we encounter a CC stream.
                hr = E_FAIL;
            }

            BREAK_ON_FAIL(hr);

            // Create the node which will represent the renderer
            hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &pOutputNode);
            BREAK_ON_FAIL(hr);

            // Store the IActivate object in the sink node - it will be extracted later by the
            // media session during the topology render phase.
            hr = pOutputNode->SetObject(pRendererActivate);
            BREAK_ON_FAIL(hr);
        }
        CComPtr<IMFMediaType> pCurrentMediaType;
        hr = pHandler->GetCurrentMediaType(&pCurrentMediaType);
        BREAK_ON_FAIL(hr);

        if (m_topoSettings.addSampleTransform)
        {
            CComPtr<IMFTopologyNode> pOldOutput = pOutputNode;
            pOutputNode = NULL;
            hr = AddSampleTransform(pCurrentMediaType, pOldOutput, &pOutputNode);
            BREAK_ON_FAIL(hr);

            pOldOutput = pOutputNode;
            pOutputNode = NULL;
            CComPtr<IMFTransform> color = CreateColorConverterMFT();
            CComPtr<IMFTopologyNode> colorConverterNode;
            hr = AddOldOutputToTopo(m_pTopology, color, pOldOutput, &colorConverterNode);
            THROW_ON_FAIL(hr);
            pOutputNode = colorConverterNode.Detach();
        }

        if(m_pNetworkSinkActivate != NULL)
        {
            CComPtr<IMFTopologyNode> pOldOutput = pOutputNode;
            pOutputNode = NULL;
            hr = CreateTeeNetworkTwig(pCurrentMediaType, pOldOutput, &pOutputNode);
            BREAK_ON_FAIL(hr);
        }
        *ppOutputNode = pOutputNode.Detach();
    }
    while(false);

    return hr;
}

HRESULT CTopoBuilder::AddSampleTransform(IMFMediaType* mediaType, IMFTopologyNode* pOldOutputNode, IMFTopologyNode** ppNewOutputNode)
{
    HRESULT hr = S_OK;
    do
    {
        CComPtr<IMFMediaType> inputMediaType;
        CComPtr<IMFMediaType> outputMediaType;
        hr = MFCreateMediaType(&inputMediaType);
        hr = MFCreateMediaType(&outputMediaType);
        CopyVideoType(mediaType, inputMediaType);
        CopyVideoType(mediaType, outputMediaType);

        CComPtr<IMFTopologyNode> sampleTransformNode;
        CComPtr<IMFTransform> sampleTransform = new (std::nothrow) SampleTransform(true);
        //hr = sampleTransform->SetInputType(0, inputMediaType, 0);
        //THROW_ON_FAIL(hr);
        //hr = sampleTransform->SetOutputType(0, outputMediaType, 0);
        //THROW_ON_FAIL(hr);

        pOldOutputNode->SetInputPrefType(0, inputMediaType);
        hr = AddOldOutputToTopo(m_pTopology, sampleTransform, pOldOutputNode, &sampleTransformNode);
        BREAK_ON_FAIL(hr);

        *ppNewOutputNode = sampleTransformNode.Detach();
    } 
    while (false);
    return hr;
}

//
// If there is a network sink, create a Tee node and hook the network sink in parallel to
// the renderer sink in the topology, then return the Tee node.
//
HRESULT CTopoBuilder::CreateTeeNetworkTwig(IMFMediaType* pSourceMediaType, 
    IMFTopologyNode* pRendererNode, IMFTopologyNode** ppTeeNode)
{
    HRESULT hr = S_OK;
    CComPtr<IMFTopologyNode> pNetworkOutputNode;
    CComPtr<IMFTopologyNode> pTeeNode;
    DWORD streamId = 0;

    do
    {
        BREAK_ON_NULL(ppTeeNode, E_POINTER);

        // if the network sink is not configured, just exit
        if(m_pNetworkSinkActivate == NULL)
            break;

        // TODO: Check how to avoid this
        //hr = pStreamDescriptor->GetStreamIdentifier(&streamId);
        streamId = 1;
        BREAK_ON_FAIL(hr);

        // create the output topology node for one of the streams on the network sink
        hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &pNetworkOutputNode);
        BREAK_ON_FAIL(hr);

        // set the output stream ID on the stream sink topology node
        hr = pNetworkOutputNode->SetUINT32(MF_TOPONODE_STREAMID, streamId);
        BREAK_ON_FAIL(hr);

        // associate the output network topology node with the network sink
        hr = pNetworkOutputNode->SetObject(m_pNetworkSinkActivate);
        BREAK_ON_FAIL(hr);

        CComPtr<IMFTopologyNode> pOutputNode = pNetworkOutputNode;

        if (m_topoSettings.addH264Encoder)
        {
            CComPtr<IMFTransform> transform;

            transform = CreateEncoderMft(pSourceMediaType, MFMediaType_Video, MEDIASUBTYPE_H264);

            CComPtr<IMFTopologyNode> transformNode;
            
            CComPtr<IMFTopologyNode> pOldOutput = pOutputNode;
            pOutputNode = NULL;

            hr = AddOldOutputToTopo(m_pTopology, transform, pOldOutput, &transformNode);

            CComPtr<IMFTransform> color = CreateColorConverterMFT();
            CComPtr<IMFTopologyNode> colorConverterNode;

            AddOldOutputToTopo(m_pTopology, color, transformNode, &colorConverterNode);
            pOutputNode = colorConverterNode.Detach();
        }
        else if (m_topoSettings.addWMWEncoder)
        {
            CComPtr<IMFMediaType> asfMediaType;
            CreateMediaTypeForAsfProfile(&asfMediaType);
            CComPtr<IMFASFContentInfo> pAsfContentInfo;
            hr = MFCreateASFContentInfo(&pAsfContentInfo);
            CComPtr<IPropertyStore> store;
            pAsfContentInfo->GetEncodingConfigurationPropertyStore(0, &store);
            CComPtr<IMFActivate> pEncoderActivate;
            hr = MFCreateWMVEncoderActivate(asfMediaType, store, &pEncoderActivate);

            CComPtr<IMFTopologyNode> asfEncoderNode;
            hr = MFCreateTopologyNode(MF_TOPOLOGY_TRANSFORM_NODE, &asfEncoderNode);
            BREAK_ON_FAIL(hr);

            hr = asfEncoderNode->SetObject(pEncoderActivate);
            CComPtr<IMFTopologyNode> pOldOutput = pOutputNode;

            hr = asfEncoderNode->SetObject(pEncoderActivate);
            THROW_ON_FAIL(hr);

            hr = asfEncoderNode->SetUINT32(MF_TOPONODE_STREAMID, 0);
            THROW_ON_FAIL(hr);

            hr = asfEncoderNode->ConnectOutput(0, pOldOutput, 0);
            m_pTopology->AddNode(pOldOutput);
            THROW_ON_FAIL(hr);

            pOutputNode = asfEncoderNode;
        }
        else
        {
            // add the network output topology node to the topology
            m_pTopology->AddNode(pNetworkOutputNode);
            BREAK_ON_FAIL(hr);
        }

        // create the topology Tee node
        hr = MFCreateTopologyNode(MF_TOPOLOGY_TEE_NODE, &pTeeNode);
        BREAK_ON_FAIL(hr);

        // connect the first Tee node output to the network sink node
        hr = pTeeNode->ConnectOutput(0, pOutputNode, 0);
        BREAK_ON_FAIL(hr);

        // if a renderer node was created and passed in, add it to the topology
        if(pRendererNode != NULL)
        {
            // add the renderer node to the topology
            hr = m_pTopology->AddNode(pRendererNode);
            BREAK_ON_FAIL(hr);

            // connect the second Tee node output to the renderer sink node
            hr = pTeeNode->ConnectOutput(1, pRendererNode, 0);
            BREAK_ON_FAIL(hr);
        }

        // detach the Tee node and return it as the output node
        *ppTeeNode = pTeeNode.Detach();
    }
    while(false);

    return hr;
}









