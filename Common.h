#pragma once

#define WIN32_LEAN_AND_MEAN
#define _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS
#include <assert.h>
#include <atlbase.h>
#include <uuids.h>
#include "Helpers.h"

using namespace std;
using namespace ATL;

static void CheckHR(HRESULT hr);

#define BREAK_ON_FAIL(value)            if(FAILED(value)) break;
#define BREAK_ON_NULL(value, newHr)     if(value == NULL) { hr = newHr; break; }


