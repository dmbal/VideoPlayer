#pragma once
#include "Common.h"

struct TopologySettings 
{
public:
    TopologySettings()
    {
        addNetwork = false;
        hWnd = NULL;
        addSampleTransform = false;
        addH264Encoder = false;
        addWMWEncoder = false;
        connectToRemoteCamera = false;
    }
    bool addNetwork;
    HWND hWnd;
    bool addSampleTransform;
    bool addH264Encoder;
    bool addWMWEncoder;
    bool connectToRemoteCamera;
};
