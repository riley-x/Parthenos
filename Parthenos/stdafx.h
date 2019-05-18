// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <string>
#include <tuple>
#include <vector>
#include <deque>
#include <unordered_set>
#include <map>
#include <cmath>
#include <ctime>
#include <exception>
#include <algorithm>
#include <numeric>


// reference additional headers your program requires here
#include <d2d1.h>
#pragma comment(lib, "d2d1")
#include <d2d1_1.h>
#include <d2d1_1helper.h>
#include <d3d11_1.h>
#pragma comment(lib, "d3d11")
#include <d2d1effects.h>
#include <d2d1effecthelpers.h>
#include <dwrite_1.h>
#pragma comment(lib, "dwrite")
#include <wrl/client.h>
#include <wincodec.h>
#include <comdef.h>
#include <shobjidl.h> 
#include <atlbase.h> 

using Microsoft::WRL::ComPtr;
